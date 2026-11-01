#pragma once
#include <cstdint>
#include <cstring>
namespace yukari {
// Native layer offsets 110/114 cursor, 118/11c anchor, 120..12c bounds.
// Evidence: 415280 glyph, 4166f0 LF, 412d80 reset (original game.exe).
struct TextFlow {
 int32_t x=0,y=0,anchor_x=0,anchor_y=0;
 int32_t left=0,top=0,right=0,bottom=0;
 int32_t cell_width=0,line_height=0;
 bool overflow=false;
 static int32_t add(int32_t a,int32_t b){uint32_t v=uint32_t(a)+uint32_t(b);int32_t n;std::memcpy(&n,&v,4);return n;}
 void reset(){x=anchor_x=left;y=anchor_y=top;overflow=false;}
 // Caller emits the glyph at (x,y) before advancing. DBCS is determined using
 // the source codepage, never by UTF-8 byte length. Proportional fonts pass the
 // original glyph bitmap width, while fixed fonts use full/half cells.
 void glyph(bool dbcs,int32_t bitmap_width){
  x=add(x,cell_width>0?(dbcs?cell_width:cell_width/2):bitmap_width);
  anchor_x=x;anchor_y=y;
  if((top==0&&bottom==0&&left==0&&right==0)||add(x,cell_width)<right)return;
  x=left;y=add(y,line_height);
  if(bottom<=add(y,line_height))overflow=true;
 }
 // True means append native zero-size type7 marker at the resulting cursor.
 // A wrap already performed by glyph() suppresses an extra line advance.
 bool lf(){
  const bool moved=x==anchor_x;
  if(moved){x=left;y=add(y,line_height);if(bottom<=add(y,line_height))overflow=true;}
  anchor_x=x;anchor_y=y;return moved;
 }
};
}
