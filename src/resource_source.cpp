#include "yukari/resource_source.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <atomic>
#include <algorithm>
#include <cstring>
#include <cerrno>
#include <limits>
#include <stdexcept>
namespace yukari {
struct ResourceSource::Impl {
 int fd=-1;std::shared_ptr<const std::vector<uint8_t>> bytes;std::atomic<uint64_t> calls{},read_bytes{};
 ~Impl(){if(fd>=0)::close(fd);}
};
ResourceSource ResourceSource::file(const std::filesystem::path& path){ResourceSource out;out.impl_=std::make_shared<Impl>();out.impl_->fd=::open(path.c_str(),O_RDONLY|O_CLOEXEC);if(out.impl_->fd<0)throw std::runtime_error("cannot open resource: "+path.string());struct stat st{};if(fstat(out.impl_->fd,&st)||!S_ISREG(st.st_mode)||st.st_size<0)throw std::runtime_error("invalid resource file: "+path.string());out.length_=uint64_t(st.st_size);return out;}
ResourceSource ResourceSource::memory(std::shared_ptr<const std::vector<uint8_t>> bytes){if(!bytes)throw std::runtime_error("null resource bytes");ResourceSource out;out.impl_=std::make_shared<Impl>();out.length_=bytes->size();out.impl_->bytes=std::move(bytes);return out;}
ResourceSource ResourceSource::slice(uint64_t offset,uint64_t length)const{if(!impl_||offset>length_||length>length_-offset)throw std::runtime_error("resource range outside source");auto out=*this;out.offset_+=offset;out.length_=length;return out;}
size_t ResourceSource::read_at(uint64_t offset,void* output,size_t count)const{
 if(!impl_||offset>length_||(!output&&count))throw std::runtime_error("invalid resource read");count=size_t(std::min<uint64_t>(count,length_-offset));if(!count)return 0;
 ++impl_->calls;size_t done=0;if(impl_->bytes){std::memcpy(output,impl_->bytes->data()+offset_+offset,count);done=count;}
 else while(done<count){auto n=::pread(impl_->fd,static_cast<char*>(output)+done,std::min(count-done,size_t(std::numeric_limits<ssize_t>::max())),off_t(offset_+offset+done));if(n<0&&errno==EINTR)continue;if(n<=0)throw std::runtime_error("truncated or unreadable resource range");done+=size_t(n);}
 impl_->read_bytes+=done;return done;
}
std::vector<uint8_t> ResourceSource::read_all()const{if(length_>512ULL*1024*1024)throw std::runtime_error("resource exceeds 512 MiB limit");std::vector<uint8_t> out(size_t(length_),uint8_t{});read_at(0,out.data(),out.size());return out;}
ResourceIoStats ResourceSource::io_stats()const{return impl_?ResourceIoStats{impl_->calls.load(),impl_->read_bytes.load()}:ResourceIoStats{};}
}
