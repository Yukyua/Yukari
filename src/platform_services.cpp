#include "yukari/platform_services.hpp"
#include <png.h>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>

namespace yukari {
namespace {
uint16_t word(const Bytes& b,size_t at){require_range(b.size(),at,2,"cursor WORD");return uint16_t(b[at]|unsigned(b[at+1])<<8);}
int number(const Value& v){if(auto n=std::get_if<int32_t>(&v))return *n;throw std::runtime_error("platform service expected integer");}
const std::string& text(const Value& v){if(auto s=std::get_if<std::string>(&v))return *s;throw std::runtime_error("platform service expected string");}
using Surface=std::unique_ptr<SDL_Surface,decltype(&SDL_FreeSurface)>;
Surface surface(const PlatformImage& image){
    if(image.width<=0||image.height<=0||image.width>4096||image.height>4096||
       uint64_t(image.width)*image.height*4!=image.rgba.size())throw std::runtime_error("invalid platform RGBA image");
    Surface result(SDL_CreateRGBSurfaceWithFormat(0,image.width,image.height,32,SDL_PIXELFORMAT_RGBA32),SDL_FreeSurface);
    if(!result)throw std::runtime_error(SDL_GetError());
    for(int y=0;y<image.height;++y)std::memcpy(static_cast<uint8_t*>(result->pixels)+y*result->pitch,
        image.rgba.data()+size_t(y)*image.width*4,size_t(image.width)*4);
    return result;
}
std::string normalized(std::string name){std::replace(name.begin(),name.end(),'\\','/');return name;}
std::filesystem::path confined(const std::filesystem::path& root,const std::string& name,bool allow_root=false){
    const auto normalized_name=normalized(name);std::filesystem::path relative(normalized_name);
    if(relative.is_absolute()||normalized_name.find(':')!=std::string::npos)throw std::runtime_error("platform output must stay inside state directory");
    const auto base=std::filesystem::weakly_canonical(std::filesystem::absolute(root));
    const auto path=std::filesystem::weakly_canonical(base/relative),under=path.lexically_relative(base);
    if(under.empty()||(!allow_root&&under==".")||(!under.empty()&&*under.begin()==".."))
        throw std::runtime_error("platform path escapes its permitted directory");
    return path;
}
}
PlatformImage decode_windows_cursor(const Bytes& bytes){
    require_range(bytes.size(),0,6,"CUR directory");const auto kind=word(bytes,2),count=word(bytes,4);
    if(word(bytes,0)!=0||(kind!=1&&kind!=2)||!count||count>256)throw std::runtime_error("unsupported CUR/ICO directory");
    require_range(bytes.size(),6,uint64_t(count)*16,"CUR entries");size_t entry=6;int score=-1;
    for(size_t i=0;i<count;++i){const auto p=6+i*16;const int w=bytes[p]?bytes[p]:256,h=bytes[p+1]?bytes[p+1]:256;
        const int candidate=10000-std::abs(w-32)-std::abs(h-32);if(candidate>score){entry=p;score=candidate;}}
    const auto at=u32(bytes,entry+12),length=u32(bytes,entry+8);require_range(bytes.size(),at,length,"CUR image");
    PlatformImage result;result.hotspot=kind==2?SDL_Point{int(word(bytes,entry+4)),int(word(bytes,entry+6))}:SDL_Point{};
    if(length>=8&&png_sig_cmp(bytes.data()+at,0,8)==0){
        png_image image{};image.version=PNG_IMAGE_VERSION;
        struct Cleanup{png_image* p;~Cleanup(){png_image_free(p);}}cleanup{&image};
        if(!png_image_begin_read_from_memory(&image,bytes.data()+at,length))throw std::runtime_error("invalid CUR PNG");
        if(!image.width||!image.height||image.width>4096||image.height>4096)throw std::runtime_error("CUR dimensions exceed limit");
        image.format=PNG_FORMAT_RGBA;result.width=int(image.width);result.height=int(image.height);result.rgba.resize(PNG_IMAGE_SIZE(image));
        if(!png_image_finish_read(&image,nullptr,result.rgba.data(),0,nullptr))throw std::runtime_error("invalid CUR PNG payload");
    }else{
        require_range(length,0,40,"CUR DIB");const auto header=u32(bytes,at);const auto raw_w=u32(bytes,at+4),raw_h=u32(bytes,at+8);
        int32_t w,h;std::memcpy(&w,&raw_w,4);std::memcpy(&h,&raw_h,4);const auto bits=word(bytes,at+14);
        if(header<40||header>length||w<=0||w>4096||h==0||int64_t(h)>8192||int64_t(h)<-8192||
           (bits!=24&&bits!=32)||u32(bytes,at+16)!=0||word(bytes,at+12)!=1)
            throw std::runtime_error("unsupported CUR DIB encoding");
        const int height=int((h<0?-int64_t(h):h)/2);if(!height)throw std::runtime_error("invalid CUR DIB height");
        const size_t stride=(size_t(w)*bits+31)/32*4,mask_stride=(size_t(w)+31)/32*4;
        require_range(length,header,stride*height+mask_stride*height,"CUR XOR/AND bitmap");
        result.width=w;result.height=height;result.rgba.resize(size_t(w)*height*4);bool has_alpha=false;
        if(bits==32)for(int y=0;y<height;++y)for(int x=0;x<w;++x)has_alpha|=bytes[at+header+size_t(y)*stride+x*4+3]!=0;
        for(int y=0;y<height;++y){const int source_y=h>0?height-y-1:y;const auto* row=bytes.data()+at+header+size_t(source_y)*stride;
            const auto* mask=bytes.data()+at+header+stride*height+size_t(source_y)*mask_stride;
            for(int x=0;x<w;++x){auto* p=result.rgba.data()+(size_t(y)*w+x)*4;p[0]=row[x*(bits/8)+2];p[1]=row[x*(bits/8)+1];p[2]=row[x*(bits/8)];
                p[3]=(mask[x/8]&(0x80>>(x%8)))?0:(has_alpha?row[x*4+3]:255);}}
    }
    if(result.hotspot.x<0||result.hotspot.y<0||result.hotspot.x>=result.width||result.hotspot.y>=result.height)
        throw std::runtime_error("CUR hotspot exceeds image bounds");
    return result;
}
struct PlatformServices::Impl{
    SDL_Window* window;SDL_Renderer* renderer;std::filesystem::path game,state;PlatformHooks hooks;
    std::array<std::unique_ptr<SDL_Cursor,decltype(&SDL_FreeCursor)>,2> cursors{{{nullptr,SDL_FreeCursor},{nullptr,SDL_FreeCursor}}};
    SDL_Point maximum;int32_t auto_hide{};bool variable{},busy{};
    Impl(SDL_Window* w,SDL_Renderer* r,std::filesystem::path g,std::filesystem::path s,PlatformHooks h)
        :window(w),renderer(r),game(std::move(g)),state(std::move(s)),hooks(std::move(h)){
        if(!window)throw std::runtime_error("platform services require an SDL window");SDL_GetWindowSize(window,&maximum.x,&maximum.y);
        if(hooks.logical_size)maximum=hooks.logical_size();variable=bool(SDL_GetWindowFlags(window)&SDL_WINDOW_RESIZABLE);
    }
    ~Impl(){for(const auto& cursor:cursors)if(cursor&&SDL_GetCursor()==cursor.get())SDL_SetCursor(SDL_GetDefaultCursor());}
    bool foreground()const{return hooks.foreground?hooks.foreground():bool(SDL_GetWindowFlags(window)&SDL_WINDOW_INPUT_FOCUS);}
    Bytes resource(const std::string& name){if(hooks.read_resource)return hooks.read_resource(name);return read_file(confined(game,name));}
    void cursor(int slot,const std::string& name){
        SDL_Cursor* value{};
        const auto& resource_name=name.empty()?hooks.default_cursor_names[size_t(slot)]:name;
        if(resource_name.empty())return;
        PlatformImage image;
        // Native42b370 keeps the previous cursor when its resource loader or
        // LoadCursorFromFile fails (including the absent default CUR in this game).
        try{image=hooks.cursor_image?hooks.cursor_image(resource_name):decode_windows_cursor(resource(resource_name));}
        catch(const std::runtime_error&){return;}
        auto pixels=surface(image);
        value=SDL_CreateColorCursor(pixels.get(),image.hotspot.x,image.hotspot.y);
        if(!value)throw std::runtime_error(std::string("cursor provider unavailable: ")+SDL_GetError());
        cursors[size_t(slot)].reset(value);if(slot==int(busy))SDL_SetCursor(value);
    }
    int32_t extract(const std::vector<Value>& args,bool dialog){
        if(args.empty())return 0;const auto& source=text(args[0]);std::string destination=args.size()>1?text(args[1]):source;
        if(dialog){if(!hooks.extract_dialog)throw std::runtime_error("FileExtractDialog requires a native save-dialog provider");
            auto selected=hooks.extract_dialog(source,confined(state,destination));if(!selected)return 0;
            if(selected->is_absolute()){auto relative=std::filesystem::weakly_canonical(*selected).lexically_relative(std::filesystem::weakly_canonical(std::filesystem::absolute(state)));destination=relative.string();}
            else destination=selected->string();}
        const auto path=confined(state,destination);Bytes bytes;try{bytes=resource(source);}catch(const std::runtime_error&){return 0;}
        std::filesystem::create_directories(path.parent_path());auto temporary=path;temporary+=".yukari-extract.tmp";
        // Exclusive/no-follow temp creation prevents a preexisting symlink from redirecting writes.
        const int fd=::open(temporary.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);if(fd<0)return 0;
        size_t written=0;while(written<bytes.size()){const auto n=::write(fd,bytes.data()+written,bytes.size()-written);if(n<=0)break;written+=size_t(n);}
        const bool complete=written==bytes.size();const int closed=::close(fd);
        if(!complete||closed!=0){std::filesystem::remove(temporary);return 0;}
        std::error_code error;std::filesystem::rename(temporary,path,error);if(error){std::filesystem::remove(temporary);return 0;}return 1;
    }
};
PlatformServices::PlatformServices(SDL_Window* w,SDL_Renderer* r,std::filesystem::path g,std::filesystem::path s,PlatformHooks h)
    :impl_(std::make_unique<Impl>(w,r,std::move(g),std::move(s),std::move(h))){}
PlatformServices::~PlatformServices()=default;
void PlatformServices::set_busy_cursor(bool busy){impl_->busy=busy;auto* cursor=impl_->cursors[busy?1:0].get();if(cursor)SDL_SetCursor(cursor);}
std::optional<Value> PlatformServices::call(const std::string& name,const std::vector<Value>& a){
    const auto null=[](){return std::optional<Value>(Value{});};
    if(name=="CursorShow"||name=="CursorHide"){if(SDL_ShowCursor(name=="CursorShow"?SDL_ENABLE:SDL_DISABLE)<0)throw std::runtime_error(SDL_GetError());return null();}
    if(name=="CursorLoad"){if(a.size()==1||a.size()==2){int slot=number(a[0]);if(slot==0||slot==1)impl_->cursor(slot,a.size()==2?text(a[1]):"");}return null();}
    if(name=="CursorAutoHide"){if(!a.empty()){const auto value=number(a[0]);if(!impl_->hooks.cursor_auto_hide)throw std::runtime_error("CursorAutoHide requires the native cursor policy provider");impl_->hooks.cursor_auto_hide(value);impl_->auto_hide=value;}return Value(impl_->auto_hide);}
    if(name=="MousePointSet"){if(a.size()>1&&impl_->foreground()){int x=number(a[0]),y=number(a[1]);if(impl_->renderer)SDL_RenderLogicalToWindow(impl_->renderer,float(x),float(y),&x,&y);SDL_WarpMouseInWindow(impl_->window,x,y);}return null();}
    if(name=="MouseXPointGet"||name=="MouseYPointGet"){int x{},y{};SDL_GetMouseState(&x,&y);float lx=float(x),ly=float(y);if(impl_->renderer)SDL_RenderWindowToLogical(impl_->renderer,x,y,&lx,&ly);return Value(int32_t(name=="MouseXPointGet"?lx:ly));}
    if(name=="MouseKeyGet"){const int key=a.empty()?1:number(a[0]);const int button=key==1?SDL_BUTTON_LEFT:key==2?SDL_BUTTON_RIGHT:key==3?SDL_BUTTON_MIDDLE:0;
        return Value(int32_t(button&&(SDL_GetMouseState(nullptr,nullptr)&SDL_BUTTON(button))!=0));}
    if(name=="WindowSizeXGet"||name=="WindowSizeYGet"){SDL_Point size{};if(impl_->hooks.logical_size)size=impl_->hooks.logical_size();else SDL_GetWindowSize(impl_->window,&size.x,&size.y);return Value(int32_t(name=="WindowSizeXGet"?size.x:size.y));}
    if(name=="WindowSizeSet"){if(a.size()>1){int w=number(a[0]),h=number(a[1]);if(w>impl_->maximum.x||h>impl_->maximum.y){w=impl_->maximum.x;h=impl_->maximum.y;}if(w<=0||h<=0)throw std::runtime_error("invalid window client size");SDL_SetWindowSize(impl_->window,w,h);}return null();}
    if(name=="WindowSizeVariableSet"){if(!a.empty()){impl_->variable=number(a[0])!=0;if(!(SDL_GetWindowFlags(impl_->window)&SDL_WINDOW_FULLSCREEN))SDL_SetWindowResizable(impl_->window,impl_->variable?SDL_TRUE:SDL_FALSE);}return Value(int32_t(impl_->variable));}
    if(name=="WindowIconChange"){if(!a.empty()){if(!impl_->hooks.icon_image)throw std::runtime_error("WindowIconChange requires original executable icon resource provider");auto image=impl_->hooks.icon_image(uint16_t(number(a[0])));auto pixels=surface(image);SDL_SetWindowIcon(impl_->window,pixels.get());}return null();}
    if(name=="WindowAutoRegionMode"){if(!a.empty()){if(!impl_->hooks.auto_region)throw std::runtime_error("WindowAutoRegionMode requires native region-shape provider");impl_->hooks.auto_region(number(a[0])!=0);}return null();}
    if(name=="FileExtract"||name=="FileExtractDialog")return Value(impl_->extract(a,name=="FileExtractDialog"));
    if(name=="Execute"){if(a.empty())return Value(int32_t(0));if(!impl_->hooks.execute)throw std::runtime_error("Execute requires a platform shell-open provider");auto cwd=confined(impl_->state,a.size()>1?text(a[1]):"",true);std::filesystem::create_directories(cwd);return Value(int32_t(impl_->hooks.execute(text(a[0]),cwd)));}
    return std::nullopt;
}
}
