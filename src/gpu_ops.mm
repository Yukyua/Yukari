#include "yukari/gpu_ops.hpp"
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include <stdexcept>
#include <cstring>
#include <string>
#include <algorithm>

namespace yukari {
namespace {
const char* shader=R"METAL(
#include <metal_stdlib>
using namespace metal;
struct Args {int p[16];int op;int w;int h;};
uint4 pixel(texture2d<float,access::read> t,uint2 xy){return uint4(rint(t.read(xy)*255.0f));}
int sar8(int n){return n>=0?n/256:-((-n+255)/256);}
uint4 blend_pixel(uint4 d,uint4 s,uint3 m,uint3 key,int type,int mmx){
   bool clear=all(m==255),solid=all(m==0);
   if(type==1)d.rgb=s.rgb;
   else if(type==3){if(any(s.rgb!=key))d.rgb=s.rgb;}
   else if(type==2){for(int c=0;c<3;++c){if(!mmx){if(!clear)d[c]=solid?s[c]:uint(int(s[c])+sar8((int(d[c])-int(s[c]))*int(m[2-c])))&255;}
    else {int delta=int(d[c])-int(s[c]);int prod=int(short(ushort(delta*int(m[c]))));int sum=clamp(prod+delta,-32768,32767);d[c]=uint(int(s[c])+(uint(ushort(short(sum)))>>8))&255;}}}
   else if(!clear)d.rgb=solid?s.rgb:type==4?min(uint3(255),d.rgb+s.rgb):uint3(max(int3(0),int3(d.rgb)+int3(s.rgb)-255));
 return d;
}
kernel void pixels(texture2d<float,access::read> a[[texture(0)]],texture2d<float,access::read> b[[texture(1)]],
 texture2d<float,access::read> mask[[texture(2)]],texture2d<float,access::write> out[[texture(3)]],
 constant Args& args[[buffer(0)]],uint2 xy[[thread_position_in_grid]]){
 if(xy.x>=uint(args.w)||xy.y>=uint(args.h))return;
 uint4 d=(args.op==5||args.op==8)?uint4(0):pixel(a,xy);int x=int(xy.x),y=int(xy.y);constant int* p=args.p;
 if(args.op==1){
  for(int c=0;c<3;++c){int v=int(d[c]);if(p[0])v=min(255,v+((p[1]>>(8*c))&255));if(p[2])v=max(0,v-((p[3]>>(8*c))&255));d[c]=uint(v);}
  if(p[4]){float lum=(float(d.b)/255.0f)*0.114f+(float(d.r)/255.0f)*0.299f+(float(d.g)/255.0f)*0.587f;
   float3 v=float3(lum+0.078512f,(lum- -0.031304f)-0.039984f,lum-0.16125199f);d.rgb=uint3(rint(clamp(v,0.0f,1.0f)*255.0f));}
  if(p[5])d.rgb=uint3((d.r*3+d.g*6+d.b)/10);if(p[6])d.rgb=255-d.rgb;
 }else if(args.op==2||args.op==6){
  if(x>=p[0]&&y>=p[1]&&x<p[0]+p[2]&&y<p[1]+p[3]){
   uint2 step=uint2((ulong(b.get_width())<<16)/uint(p[2]),(ulong(b.get_height())<<16)/uint(p[3]));uint2 src=uint2((ulong(x-p[0])*step.x+step.x/2)>>16,(ulong(y-p[1])*step.y+step.y/2)>>16);uint4 s=pixel(b,src);
   if(args.op==6){uint tail=(d.a*(255-s.a)+127)/255,alpha=s.a+tail;if(alpha)d.rgb=(s.rgb*s.a+d.rgb*tail+alpha/2)/alpha;d.a=alpha;}
   else {uint alpha=s.a*255+d.a*(255-s.a);d.rgb=alpha?(s.rgb*s.a*255+d.rgb*d.a*(255-s.a)+alpha/2)/alpha:uint3(0);d.a=(alpha+127)/255;}
  }
 }else if(args.op==9){uint3 m=pixel(b,xy).rgb;uint opacity=255-(m.r+m.g+m.b)/3;d.a=(d.a*opacity+127)/255;
 }else if(args.op==3){if(x>=p[0]&&y>=p[1]&&x<p[0]+p[2]&&y<p[1]+p[3])d.rgb=uint3(p[4]&255,(p[4]>>8)&255,(p[4]>>16)&255);
 }else if(args.op==4){
  if(x>=p[0]&&y>=p[1]&&x<p[0]+p[4]&&y<p[1]+p[5]){
   uint2 src=uint2(x-p[0]+p[2],y-p[1]+p[3]);uint4 s=pixel(b,src);uint3 m=p[8]?pixel(mask,src).rgb:uint3(255-s.a);
   d=blend_pixel(d,s,m,pixel(b,uint2(0,b.get_height()-1)).rgb,p[6],p[7]);
  }
 }else if(args.op==5){
  int mode=p[0],v=0,w=args.w;
  if(p[1])v=int(pixel(b,xy).b);
  else if(mode==1){const int order[16]={0,8,3,10,12,4,14,7,2,9,1,11,15,6,13,5};v=order[(y%4)*4+x%4]*15;}
  else if(mode==3)v=(w-x)*255/w;else if(mode==4)v=x*255/w;
  else if(mode==5)v=(w-y)*255/w;else if(mode==6)v=y*255/w;
  else if(mode>=7&&mode<=10){int q=(mode<=8?x:y)%16;v=int(float(mode%2?16-q:q)*15.9375f);}
  else if(mode>=11&&mode<=14){int q=mode<=12?x:y,j=(16-q%16)%16,k=q+17*j;v=int(float(k%(w+256))*255.0f/float(w+256));if(mode%2)v=255-v;}
  d=uint4(255,255,255,v&255);
 }
 out.write(float4(d)/255.0f,xy);
}
// Self-copy is ordered in native bottom-up rows, then left-to-right columns.
// A single GPU lane deliberately retains read-after-write aliasing semantics.
uint4 unpack(uint p){return uint4(p&255,(p>>8)&255,(p>>16)&255,p>>24);}
uint pack(uint4 p){return p.r|(p.g<<8)|(p.b<<16)|(p.a<<24);}
kernel void self_blit(texture2d<float,access::read> a[[texture(0)]],texture2d<float,access::read> mask[[texture(2)]],
 texture2d<float,access::write> out[[texture(3)]],constant Args& args[[buffer(0)]],device uint* scratch[[buffer(1)]],uint2 lane[[thread_position_in_grid]]){
 if(any(lane!=uint2(0)))return;constant int* p=args.p;
 for(int y=0;y<args.h;++y)for(int x=0;x<args.w;++x)scratch[y*args.w+x]=pack(pixel(a,uint2(x,y)));
 uint3 key=unpack(scratch[(args.h-1)*args.w]).rgb;
 for(int row=p[5]-1;row>=0;--row)for(int col=0;col<p[4];++col){
  uint2 src=uint2(p[2]+col,p[3]+row),dst=uint2(p[0]+col,p[1]+row);
  uint4 s=unpack(scratch[src.y*args.w+src.x]),d=unpack(scratch[dst.y*args.w+dst.x]);uint3 m=p[8]?pixel(mask,src).rgb:uint3(255-s.a);
  scratch[dst.y*args.w+dst.x]=pack(blend_pixel(d,s,m,key,p[6],p[7]));
 }
 for(int y=0;y<args.h;++y)for(int x=0;x<args.w;++x)out.write(float4(unpack(scratch[y*args.w+x]))/255.0f,uint2(x,y));
}
kernel void text_over(texture2d<float,access::read> source[[texture(1)]],texture2d<float,access::read_write> out[[texture(3)]],constant Args& args[[buffer(0)]],uint2 lane[[thread_position_in_grid]]){
 constant int* p=args.p;int2 dst=int2(lane)+int2(max(0,p[0]),max(0,p[1]));int2 src=dst-int2(p[0],p[1]);
 if(dst.x>=args.w||dst.y>=args.h||src.x>=p[2]||src.y>=p[3])return;
 uint4 s=pixel(source,uint2(src)),d=uint4(rint(out.read(uint2(dst))*255.0f));uint tail=(d.a*(255-s.a)+127)/255,alpha=s.a+tail;
 if(s.a){d.rgb=(s.rgb*s.a+d.rgb*tail+alpha/2)/alpha;d.a=alpha;out.write(float4(d)/255.0f,uint2(dst));}
}
struct Vertex {float4 position[[position]];float2 uv;};
vertex Vertex image_vertex(uint id[[vertex_id]]){float2 p=float2((id<<1)&2,id&2);return {float4(p.x*2-1,1-p.y*2,0,1),p};}
fragment float4 image_fragment(Vertex v[[stage_in]],texture2d<float> image[[texture(0)]]){constexpr sampler nearest(coord::normalized,filter::nearest,address::clamp_to_edge);return image.sample(nearest,v.uv);}
)METAL";
void failure(NSError* error,const char* what){throw std::runtime_error(std::string(what)+": "+(error?[[error localizedDescription] UTF8String]:"Metal returned nil"));}
}
struct GpuPixels::Impl {id<MTLTexture> texture;explicit Impl(id<MTLTexture> t):texture(t){}~Impl(){[texture release];}};
struct GpuOps::Impl {
 SDL_Renderer* renderer;id<MTLDevice> device;id<MTLCommandQueue> queue;id<MTLComputePipelineState> compute,self_copy,text_over;id<MTLCommandBuffer> last=nil,pending=nil;id<MTLRenderPipelineState> copy;
 Impl(SDL_Renderer* r):renderer(r){
  CAMetalLayer* layer=(CAMetalLayer*)SDL_RenderGetMetalLayer(r);device=[layer.device retain];if(!device)throw std::runtime_error("SDL Metal device unavailable");
  queue=[device newCommandQueue];NSError* error=nil;MTLCompileOptions* options=[[MTLCompileOptions alloc]init];if(@available(macOS 15.0,*))options.mathMode=MTLMathModeSafe;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  else options.fastMathEnabled=NO;
#pragma clang diagnostic pop
  id<MTLLibrary> library=[device newLibraryWithSource:[NSString stringWithUTF8String:shader] options:options error:&error];[options release];if(!library)failure(error,"Metal image shader");
  id<MTLFunction> fn=[library newFunctionWithName:@"pixels"];compute=[device newComputePipelineStateWithFunction:fn error:&error];[fn release];if(!compute){[library release];failure(error,"Metal image compute");}
  fn=[library newFunctionWithName:@"self_blit"];self_copy=[device newComputePipelineStateWithFunction:fn error:&error];[fn release];if(!self_copy){[library release];failure(error,"Metal self-copy compute");}
  fn=[library newFunctionWithName:@"text_over"];text_over=[device newComputePipelineStateWithFunction:fn error:&error];[fn release];if(!text_over){[library release];failure(error,"Metal text-over compute");}
  MTLRenderPipelineDescriptor* desc=[[MTLRenderPipelineDescriptor alloc]init];desc.vertexFunction=[library newFunctionWithName:@"image_vertex"];desc.fragmentFunction=[library newFunctionWithName:@"image_fragment"];desc.colorAttachments[0].pixelFormat=MTLPixelFormatRGBA8Unorm;
  copy=[device newRenderPipelineStateWithDescriptor:desc error:&error];[desc.vertexFunction release];[desc.fragmentFunction release];[desc release];[library release];if(!copy)failure(error,"Metal image copy");
 }
 void flush(){if(pending){[pending commit];[last release];last=pending;pending=nil;}}
 ~Impl(){flush();[last waitUntilCompleted];[last release];[self_copy release];[text_over release];[copy release];[compute release];[queue release];[device release];}
 std::shared_ptr<GpuPixels> image(int w,int h){MTLTextureDescriptor* d=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm width:w height:h mipmapped:NO];d.usage=MTLTextureUsageShaderRead|MTLTextureUsageShaderWrite;d.storageMode=MTLStorageModeShared;auto t=[device newTextureWithDescriptor:d];if(!t)throw std::runtime_error("Metal image allocation failed");auto out=std::make_shared<GpuPixels>();out->width=w;out->height=h;out->impl=std::make_shared<GpuPixels::Impl>(t);return out;}
};
GpuOps::GpuOps(SDL_Renderer* r):impl_(std::make_unique<Impl>(r)){}
GpuOps::~GpuOps()=default;
std::unique_ptr<GpuOps> GpuOps::create(SDL_Renderer* r){SDL_RendererInfo info{};if(SDL_GetRendererInfo(r,&info)||std::strcmp(info.name,"metal"))return {};return std::unique_ptr<GpuOps>(new GpuOps(r));}
std::shared_ptr<GpuPixels> GpuOps::upload(const SDL_Surface* pixels){@autoreleasepool{auto image=impl_->image(pixels->w,pixels->h);[image->impl->texture replaceRegion:MTLRegionMake2D(0,0,pixels->w,pixels->h) mipmapLevel:0 withBytes:pixels->pixels bytesPerRow:pixels->pitch];return image;}}
std::shared_ptr<GpuPixels> GpuOps::run(Operation op,const std::shared_ptr<GpuPixels>& destination,const std::shared_ptr<GpuPixels>& source,const std::shared_ptr<GpuPixels>& mask,const std::array<int,16>& p,int width,int height){@autoreleasepool{
 if(op==TextOver&&(p[0]>=width||p[1]>=height||int64_t(p[0])+p[2]<=0||int64_t(p[1])+p[3]<=0))return destination;
 auto out=op==TextOver?destination:impl_->image(width,height);auto a=destination?destination:out,b=source?source:a,m=mask?mask:a;
 struct Args {int p[16];int op,w,h;} args{};std::copy(p.begin(),p.end(),args.p);args.op=op;args.w=width;args.h=height;
 // Keep ordered compute encoders in one command buffer until an SDL export
 // or host read needs their results. Metal tracks inter-encoder dependencies.
 if(!impl_->pending)impl_->pending=[[impl_->queue commandBuffer] retain];
 id<MTLCommandBuffer> commands=impl_->pending;id<MTLComputeCommandEncoder> encoder=[commands computeCommandEncoder];[encoder setComputePipelineState:op==GpuOps::SelfBlit?impl_->self_copy:op==GpuOps::TextOver?impl_->text_over:impl_->compute];[encoder setTexture:a->impl->texture atIndex:0];[encoder setTexture:b->impl->texture atIndex:1];[encoder setTexture:m->impl->texture atIndex:2];[encoder setTexture:out->impl->texture atIndex:3];[encoder setBytes:&args length:sizeof args atIndex:0];
 id<MTLBuffer> scratch=nil;if(op==GpuOps::SelfBlit){scratch=[impl_->device newBufferWithLength:size_t(width)*height*4 options:MTLResourceStorageModePrivate];if(!scratch)throw std::runtime_error("Metal self-copy scratch allocation failed");[encoder setBuffer:scratch offset:0 atIndex:1];}
 const int dispatch_w=op==TextOver?std::min(width,p[0]+p[2])-std::max(0,p[0]):width,dispatch_h=op==TextOver?std::min(height,p[1]+p[3])-std::max(0,p[1]):height;
 [encoder dispatchThreads:op==GpuOps::SelfBlit?MTLSizeMake(1,1,1):MTLSizeMake(dispatch_w,dispatch_h,1) threadsPerThreadgroup:op==GpuOps::SelfBlit?MTLSizeMake(1,1,1):MTLSizeMake(8,8,1)];[encoder endEncoding];[scratch release];return out;
}}
void GpuOps::write_target(SDL_Texture* target,const std::shared_ptr<GpuPixels>& image){@autoreleasepool{
 impl_->flush();[impl_->last waitUntilCompleted];if(impl_->last.status==MTLCommandBufferStatusError)failure(impl_->last.error,"Metal image operation");
 if(SDL_GetRenderTarget(impl_->renderer)!=target)throw std::runtime_error("Metal image export target is not selected");if(SDL_RenderFlush(impl_->renderer))throw std::runtime_error(SDL_GetError());
 id<MTLRenderCommandEncoder> encoder=(id<MTLRenderCommandEncoder>)SDL_RenderGetMetalCommandEncoder(impl_->renderer);if(!encoder)throw std::runtime_error("SDL Metal target encoder unavailable");
 [encoder setRenderPipelineState:impl_->copy];[encoder setViewport:MTLViewport{0,0,double(image->width),double(image->height),0,1}];[encoder setScissorRect:MTLScissorRect{0,0,NSUInteger(image->width),NSUInteger(image->height)}];[encoder setCullMode:MTLCullModeNone];[encoder setFragmentTexture:image->impl->texture atIndex:0];[encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
}}
void GpuOps::read(const std::shared_ptr<GpuPixels>& image,SDL_Surface* output){impl_->flush();[impl_->last waitUntilCompleted];if(impl_->last.status==MTLCommandBufferStatusError)failure(impl_->last.error,"Metal image readback");if(output->w!=image->width||output->h!=image->height)throw std::runtime_error("Metal readback dimensions differ");[image->impl->texture getBytes:output->pixels bytesPerRow:output->pitch fromRegion:MTLRegionMake2D(0,0,image->width,image->height) mipmapLevel:0];}
}
