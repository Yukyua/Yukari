#include "yukari/native_text_box.hpp"
#import <AppKit/AppKit.h>
#include <SDL_syswm.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace yukari {
namespace {
void main_thread(){if(![NSThread isMainThread])throw std::runtime_error("native TextBox requires the AppKit main thread");}
NSString* ns(const std::string& input){auto end=input.find('\0');auto size=end==std::string::npos?input.size():end;NSString* value=[[[NSString alloc] initWithBytes:input.data() length:size encoding:NSUTF8StringEncoding] autorelease];if(!value)throw std::runtime_error("TextBox requires valid UTF-8");return value;}
int32_t virtual_key(NSEvent* e){
 switch([e keyCode]){case 36:case 76:return 13;case 53:return 27;case 48:return 9;case 51:return 8;case 117:return 46;case 123:return 37;case 126:return 38;case 124:return 39;case 125:return 40;
 case 122:return 112;case 120:return 113;case 99:return 114;case 118:return 115;case 96:return 116;case 97:return 117;case 98:return 118;case 100:return 119;case 101:return 120;case 109:return 121;case 103:return 122;case 111:return 123;
 case 82:return 96;case 83:return 97;case 84:return 98;case 85:return 99;case 86:return 100;case 87:return 101;case 88:return 102;case 89:return 103;case 91:return 104;case 92:return 105;default:break;}
 NSString* text=[e charactersIgnoringModifiers];if(![text length])return 0;unichar c=[text characterAtIndex:0];if(c>='a'&&c<='z')return c-'a'+'A';if(c>=32&&c<127)return c;return 0;
}
SDL_Keycode sdl_key(int32_t vk){if(vk>='A'&&vk<='Z')return SDL_Keycode(vk-'A'+'a');if(vk>=112&&vk<=123)return SDL_Keycode(SDLK_F1+vk-112);if(vk>=32&&vk<127)return SDL_Keycode(vk);switch(vk){case 13:return SDLK_RETURN;case 27:return SDLK_ESCAPE;case 9:return SDLK_TAB;case 8:return SDLK_BACKSPACE;case 37:return SDLK_LEFT;case 38:return SDLK_UP;case 39:return SDLK_RIGHT;case 40:return SDLK_DOWN;case 46:return SDLK_DELETE;default:return SDLK_UNKNOWN;}}
void post_sdl_key(SDL_Window* window,int32_t vk,bool down,bool repeat,NSEventModifierFlags flags){auto key=sdl_key(vk);if(key==SDLK_UNKNOWN)return;SDL_Event e{};e.type=down?SDL_KEYDOWN:SDL_KEYUP;e.key.windowID=SDL_GetWindowID(window);e.key.state=down?SDL_PRESSED:SDL_RELEASED;e.key.repeat=repeat;e.key.keysym.sym=key;e.key.keysym.scancode=SDL_GetScancodeFromKey(key);int mod=KMOD_NONE;if(flags&NSEventModifierFlagShift)mod|=KMOD_SHIFT;if(flags&NSEventModifierFlagControl)mod|=KMOD_CTRL;if(flags&NSEventModifierFlagOption)mod|=KMOD_ALT;if(flags&NSEventModifierFlagCommand)mod|=KMOD_GUI;e.key.keysym.mod=Uint16(mod);SDL_PushEvent(&e);}
bool native_forwarded_key(int32_t vk){return vk&&vk!=8&&vk!=32&&vk!=46&&!(vk>=37&&vk<=40)&&!(vk>=96&&vk<=105);}
}
struct NativeTextBox::Impl {
 SDL_Window* sdl;int logical_w,logical_h;FontFamily font;KeyForwarder forward;BlockedKey blocked;
 NSWindow* window=nil;NSView* parent=nil;NSTextField* field=nil;id key_observer=nil,resize_observer=nil;TextBoxSpec spec{};
 Impl(SDL_Window* w,int x,int y,FontFamily f,KeyForwarder k,BlockedKey b):sdl(w),logical_w(x),logical_h(y),font(std::move(f)),forward(std::move(k)),blocked(std::move(b)){}
 void update(){
  if(!field)return;auto bounds=[parent bounds];double scale=std::min(bounds.size.width/logical_w,bounds.size.height/logical_h);if(scale<=0)return;
  double ox=(bounds.size.width-logical_w*scale)/2,oy=(bounds.size.height-logical_h*scale)/2;
  double x=ox+spec.value_28*scale,y=oy+spec.value_2c*scale,w=spec.value_24*scale,h=spec.font_height*scale;
  if(![parent isFlipped])y=bounds.size.height-y-h;
  [field setFrame:NSMakeRect(x,y,w,h)];
  NSString* family=font?ns(font()):@"";CGFloat target=std::max(1.0,std::abs(double(spec.font_height))*scale);NSFont* face=[family length]?[NSFont fontWithName:family size:target]:nil;if(!face)face=[NSFont systemFontOfSize:target];
  const CGFloat cell=[face ascender]-[face descender]+[face leading];if(cell>0){NSFont* fitted=[NSFont fontWithName:[face fontName] size:target*target/cell];if(fitted)face=fitted;}[field setFont:face];
 }
 void remove(){
  if(key_observer){[NSEvent removeMonitor:key_observer];[key_observer release];key_observer=nil;}
  if(resize_observer){[[NSNotificationCenter defaultCenter] removeObserver:resize_observer];[resize_observer release];resize_observer=nil;}
  if(field){NSText* editor=[field currentEditor];if(editor&&[window firstResponder]==editor)[window makeFirstResponder:parent];[field removeFromSuperview];[field release];field=nil;}
  if(parent){[parent release];parent=nil;}if(window){[window release];window=nil;}
 }
};
NativeTextBox::NativeTextBox(SDL_Window* window,int w,int h,FontFamily family,KeyForwarder key,BlockedKey blocked):impl_(std::make_unique<Impl>(window,w,h,std::move(family),std::move(key),std::move(blocked))){if(!window||w<=0||h<=0)throw std::runtime_error("invalid native TextBox parent/logical dimensions");}
NativeTextBox::~NativeTextBox(){if(impl_->field||impl_->window||impl_->parent){@autoreleasepool{impl_->remove();}}}
void NativeTextBox::bind(TextModelHooks& h){h.create_box=[this](const TextBoxSpec& s){create(s);};h.delete_box=[this]{destroy();};h.get_box=[this]{return get();};h.set_box=[this](const std::string& s){set(s);};}
void NativeTextBox::create(const TextBoxSpec& spec){
 if(spec.value_24<=0||spec.font_height<=0||spec.value_24>65536||spec.font_height>65536)throw std::runtime_error("native TextBox dimensions outside safe profile");
 SDL_SysWMinfo info{};SDL_VERSION(&info.version);if(!SDL_GetWindowWMInfo(impl_->sdl,&info)||info.subsystem!=SDL_SYSWM_COCOA)throw std::runtime_error("native TextBox requires an SDL Cocoa window");
 main_thread();@autoreleasepool{
  impl_->remove();impl_->spec=spec;impl_->window=[info.info.cocoa.window retain];impl_->parent=[[impl_->window contentView] retain];
  impl_->field=[[NSTextField alloc] initWithFrame:NSZeroRect];if(!impl_->field){impl_->remove();throw std::runtime_error("cannot allocate native TextBox");}
  [impl_->field setEditable:YES];[impl_->field setSelectable:YES];[impl_->field setBordered:NO];[impl_->field setBezeled:NO];[impl_->field setDrawsBackground:NO];[impl_->field setUsesSingleLineMode:YES];[impl_->field setContinuous:YES];
  uint32_t c=uint32_t(spec.style);[impl_->field setTextColor:[NSColor colorWithSRGBRed:(c&255)/255.0 green:((c>>8)&255)/255.0 blue:((c>>16)&255)/255.0 alpha:1]];
  [[impl_->field cell] setScrollable:NO];[[impl_->field cell] setLineBreakMode:NSLineBreakByClipping];[[impl_->field cell] setWraps:NO];
  [impl_->parent addSubview:impl_->field positioned:NSWindowAbove relativeTo:nil];impl_->update();[impl_->window makeFirstResponder:impl_->field];
  auto* self=impl_.get();
  impl_->key_observer=[[NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyDown|NSEventMaskKeyUp handler:^NSEvent*(NSEvent* event){
   if([event window]!=self->window||![self->field currentEditor]||[self->window firstResponder]!=[self->field currentEditor])return event;
   id editor=[self->field currentEditor];if([editor respondsToSelector:@selector(hasMarkedText)]&&[editor hasMarkedText])return event;
   int32_t vk=virtual_key(event);if(native_forwarded_key(vk)&&(!self->blocked||!self->blocked(vk))){bool down=[event type]==NSEventTypeKeyDown,repeat=[event isARepeat];if(self->forward)self->forward(vk,down,repeat);else post_sdl_key(self->sdl,vk,down,repeat,[event modifierFlags]);}return event;
  }] retain];
  [impl_->parent setPostsFrameChangedNotifications:YES];
  impl_->resize_observer=[[[NSNotificationCenter defaultCenter] addObserverForName:NSViewFrameDidChangeNotification object:impl_->parent queue:nil usingBlock:^(NSNotification*){self->update();}] retain];
 }
}
void NativeTextBox::destroy(){main_thread();@autoreleasepool{impl_->remove();}}
void NativeTextBox::set(const std::string& utf8){main_thread();@autoreleasepool{if(!impl_->field)throw std::runtime_error("TextBoxStringSet has no live native edit control");NSString* value=ns(utf8);[impl_->field setStringValue:value];NSText* editor=[impl_->field currentEditor];if(editor){[editor setString:value];[editor setSelectedRange:NSMakeRange(0,0)];}}}
std::string NativeTextBox::get()const{main_thread();@autoreleasepool{if(!impl_->field)throw std::runtime_error("TextBoxStringGet has no live native edit control");NSText* editor=[impl_->field currentEditor];NSString* value=editor?[editor string]:[impl_->field stringValue];NSData* bytes=[value dataUsingEncoding:NSUTF8StringEncoding allowLossyConversion:NO];if(!bytes)throw std::runtime_error("native TextBox contains invalid Unicode");if(![bytes length])return {};return std::string(static_cast<const char*>([bytes bytes]),[bytes length]);}}
bool NativeTextBox::exists()const{return impl_->field!=nil;}
SDL_Rect NativeTextBox::geometry()const{const auto& s=impl_->spec;return {s.value_28,s.value_2c,s.value_24,s.font_height};}
void NativeTextBox::sync_geometry(){main_thread();@autoreleasepool{impl_->update();}}
}
