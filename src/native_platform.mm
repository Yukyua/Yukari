#include "yukari/native_platform.hpp"
#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>
#import <CoreText/CoreText.h>
#include <SDL_syswm.h>
#import <QuartzCore/QuartzCore.h>
#include <stdexcept>
@interface YukariMenuSelection : NSObject { @public NSInteger selected; }
- (void)choose:(id)sender;
@end
@implementation YukariMenuSelection
- (void)choose:(id)sender { selected=[sender tag]; }
@end
namespace yukari {
namespace {NSString* ns(const std::string& value){return [[[NSString alloc] initWithBytes:value.data() length:value.size() encoding:NSUTF8StringEncoding] autorelease];}}
std::optional<std::filesystem::path> native_extract_dialog(const std::string& source,const std::filesystem::path& suggested){
 @autoreleasepool {
  NSSavePanel* panel=[NSSavePanel savePanel];[panel setTitle:@"Yukari — FileExtract"];[panel setMessage:ns(source)];[panel setCanCreateDirectories:YES];[panel setDirectoryURL:[NSURL fileURLWithPath:ns(suggested.parent_path().string()) isDirectory:YES]];[panel setNameFieldStringValue:ns(suggested.filename().string())];
  if([panel runModal]!=NSModalResponseOK)return std::nullopt;
  const char* path=[[[panel URL] path] fileSystemRepresentation];if(!path)return std::nullopt;return std::filesystem::path(path);
 }
}
std::optional<size_t> native_popup_menu(SDL_Window* window,SDL_Point point,const std::vector<std::string>& labels){
 @autoreleasepool {
  SDL_SysWMinfo info{};SDL_VERSION(&info.version);if(!SDL_GetWindowWMInfo(window,&info)||info.subsystem!=SDL_SYSWM_COCOA)return std::nullopt;
  NSWindow* native=info.info.cocoa.window;NSView* view=[native contentView];
  NSMenu* menu=[[[NSMenu alloc] initWithTitle:@"Yukari"] autorelease];YukariMenuSelection* target=[[[YukariMenuSelection alloc] init] autorelease];target->selected=-1;
  for(size_t i=0;i<labels.size();++i){NSMenuItem* item=[[[NSMenuItem alloc] initWithTitle:ns(labels[i]) action:@selector(choose:) keyEquivalent:@""] autorelease];[item setTarget:target];[item setTag:NSInteger(i)];[menu addItem:item];}
  [menu popUpMenuPositioningItem:nil atLocation:NSMakePoint(point.x,[view bounds].size.height-point.y) inView:view];if(target->selected<0)return std::nullopt;return size_t(target->selected);
 }
}
std::vector<std::string> native_font_families(){
 @autoreleasepool {std::vector<std::string> result;for(NSString* family in [[NSFontManager sharedFontManager] availableFontFamilies]){const char* value=[family UTF8String];if(value)result.emplace_back(value);}return result;}
}
std::filesystem::path native_font_file(const std::string& family){
 @autoreleasepool {NSString* name=ns(family);if(!name)return {};CTFontRef font=CTFontCreateWithName((CFStringRef)name,27,nullptr);if(!font)return {};CFTypeRef attribute=CTFontCopyAttribute(font,kCTFontURLAttribute);CFRelease(font);std::filesystem::path result;if(attribute){if(CFGetTypeID(attribute)==CFURLGetTypeID()){const char* path=[[(NSURL*)attribute path] fileSystemRepresentation];if(path)result=path;}CFRelease(attribute);}return result;}
}
void native_window_region(SDL_Window* window,const Bytes& png){
 @autoreleasepool {
  SDL_SysWMinfo info{};SDL_VERSION(&info.version);if(!SDL_GetWindowWMInfo(window,&info)||info.subsystem!=SDL_SYSWM_COCOA)throw std::runtime_error("window region requires Cocoa");NSWindow* native=info.info.cocoa.window;NSView* view=[native contentView];
  if(png.empty()){[[view layer] setMask:nil];[native setOpaque:YES];[native setBackgroundColor:[NSColor windowBackgroundColor]];return;}
  NSBitmapImageRep* bitmap=[NSBitmapImageRep imageRepWithData:[NSData dataWithBytes:png.data() length:png.size()]];if(!bitmap)throw std::runtime_error("cannot decode region bitmap");NSInteger w=[bitmap pixelsWide],h=[bitmap pixelsHigh];if(w<=0||h<=0||w>16384||h>16384)throw std::runtime_error("invalid region bitmap dimensions");
  // Original 41a560 uses the first DWORD of a bottom-up DIB as color key.
  NSColor* key=[[bitmap colorAtX:0 y:h-1] colorUsingColorSpace:[NSColorSpace sRGBColorSpace]];auto channel=[](CGFloat v){return int(v*255+.5);};int red=channel([key redComponent]),green=channel([key greenComponent]),blue=channel([key blueComponent]);
  NSBitmapImageRep* mask=[[[NSBitmapImageRep alloc] initWithBitmapDataPlanes:nullptr pixelsWide:w pixelsHigh:h bitsPerSample:8 samplesPerPixel:4 hasAlpha:YES isPlanar:NO colorSpaceName:NSDeviceRGBColorSpace bytesPerRow:w*4 bitsPerPixel:32] autorelease];auto* data=[mask bitmapData];
  for(NSInteger y=0;y<h;++y)for(NSInteger x=0;x<w;++x){NSColor* color=[[bitmap colorAtX:x y:y] colorUsingColorSpace:[NSColorSpace sRGBColorSpace]];bool opaque=channel([color redComponent])!=red||channel([color greenComponent])!=green||channel([color blueComponent])!=blue;size_t at=size_t(y*w+x)*4;data[at]=data[at+1]=data[at+2]=255;data[at+3]=opaque?255:0;}
  [view setWantsLayer:YES];CALayer* region=[CALayer layer];[region setFrame:[view bounds]];[region setContents:(id)[mask CGImage]];[region setContentsGravity:kCAGravityResize];[[view layer] setMask:region];[native setOpaque:NO];[native setBackgroundColor:[NSColor clearColor]];
 }
}
bool native_shell_open(const std::string& target,const std::filesystem::path& cwd){
 @autoreleasepool {
  NSString* name=ns(target);if(!name)return false;
  NSURL* url=[NSURL URLWithString:name];
  // ShellExecute receives a single file/URL target, not a shell command line.
  if(![url scheme]||[[url scheme] length]<2){auto path=std::filesystem::path(target);if(path.is_relative())path=cwd/path;url=[NSURL fileURLWithPath:ns(path.lexically_normal().string())];
   if([[NSFileManager defaultManager] isExecutableFileAtPath:[url path]]&&![[[url path] pathExtension] isEqualToString:@"app"]){
    NSTask* task=[[[NSTask alloc] init] autorelease];[task setExecutableURL:url];[task setCurrentDirectoryURL:[NSURL fileURLWithPath:ns(cwd.string()) isDirectory:YES]];NSError* error=nil;return [task launchAndReturnError:&error];
   }
  }
  return [[NSWorkspace sharedWorkspace] openURL:url];
 }
}
}
