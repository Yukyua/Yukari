#include "yukari/text_flow.hpp"
#include <cassert>
#include <climits>
#include <iostream>
using namespace yukari;
int main(){
 TextFlow t;t.left=10;t.top=20;t.right=110;t.bottom=140;t.cell_width=20;t.line_height=30;t.reset();
 assert(t.x==10&&t.y==20&&!t.overflow);t.glyph(true,99);assert(t.x==30&&t.anchor_x==30);t.glyph(false,99);assert(t.x==40);assert(t.lf()&&t.x==10&&t.y==50);
 t.glyph(true,1);t.glyph(true,1);t.glyph(true,1);t.glyph(true,1);assert(t.x==10&&t.y==80&&t.anchor_x==90);assert(!t.lf()&&t.y==80&&t.anchor_x==10);assert(t.lf()&&t.y==110&&t.overflow);
 t.reset();assert(!t.overflow&&t.anchor_y==20);t.cell_width=0;t.glyph(false,17);assert(t.x==27);t.glyph(true,13);assert(t.x==40);
 TextFlow unbounded;unbounded.cell_width=20;unbounded.line_height=30;unbounded.glyph(true,0);assert(unbounded.x==20&&unbounded.y==0);assert(unbounded.lf()&&unbounded.y==30&&unbounded.overflow);
 assert(TextFlow::add(INT_MAX,1)==INT_MIN);std::cout<<"native text glyph advance, wrap, LF and PF cursor reset passed\n";
}
