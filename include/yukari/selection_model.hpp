#pragma once
#include "yukari/vm.hpp"
#include <array>
#include <utility>
namespace yukari {
struct SelectionRect { int32_t left=INT32_MIN,top=INT32_MIN,right=INT32_MAX,bottom=INT32_MAX; bool contains(int32_t x,int32_t y)const; };
struct SelectionSprite { int32_t parent=-1,source=-1,x=0,y=0,group=-1,delay_ms=0; bool present()const{return group>=0;} };
struct SelectionItem {
 enum Kind { Mouse=0,MouseKey=1,Key=2,KeyHistory=3,Message=4,Indicate=5,Together=6,Selecting=7,SelectingAlternate=8,MoveIndex=9,MoveAuto=10 } kind=Mouse;
 uint32_t branch=0,end=0;SelectionRect rect;
 bool blocked=false,allow_repeat=false,stop_hit_scan=false;
 bool navigation_excluded=false,ignore_input=false,key_over_prev=false,key_over_next=false;
 bool clickable_cursor=false;
 bool native_c9=false;
 int32_t relative_target=0;
 std::array<std::optional<size_t>,4> neighbors; // up, down, left, right
 int32_t mouse_key=1,message=0;
 std::vector<int32_t> keys;
 std::array<SelectionSprite,3> sprites; // normal, indicated, disabled
 int32_t enter_sound=-1,select_sound=-1;
 bool loop_enter_sound=false;
 std::string text; // Case2: host lays out normal and indicated text sprites.
};
struct SelectionRegistration {
 std::optional<SelectionRect> text_rect;
 bool already=false;
 int32_t text_render_mode=0;
 std::optional<std::pair<int32_t,int32_t>> text_position;
};
enum class SelectionRegistrationResult { Added, ConditionFalse, NativeUnrecognized, Unhandled };
SelectionRegistrationResult decode_selection_case(const HostCall&,SelectionItem&,const SelectionRegistration& options={});
struct SelectionInput {
 enum Query { MouseInput,KeyDown,KeyHistoryChanged,WindowMessage,VisualOnly,SelectingKey } query=VisualOnly;
 uint32_t message=0,wparam=0,lparam=0;
 int32_t x=0,y=0;
 std::array<bool,3> mouse_held{};
 std::optional<size_t> focused;
 std::array<uint16_t,256> key_state{}; // raw original GetKeyState SHORT values
 // Native selector's most-recent-first key history, supplied by host's window
 // input collector. A KeyTableCase is an ordered sequence, not a chord.
 std::array<int32_t,16> key_history{};
};
struct SelectionNavigationOptions {
 int32_t mode=1; // Native 41acf0 initializes index navigation.
 std::vector<int32_t> previous{0x26,0x25,0x68,0x64},next{0x28,0x27,0x62,0x66};
 std::vector<int32_t> up{0x26,0x68},down{0x28,0x62},left{0x25,0x64},right{0x27,0x66};
 bool tab_groups=false;
};
struct SelectionNavigationResult {std::optional<size_t> focused;bool activate=false;};
struct SelectionCandidate { size_t item;uint32_t branch,end; };
struct SelectionChange { size_t item;unsigned slot;bool visible,reset; };
struct SelectionUpdate { std::optional<SelectionCandidate> candidate;std::vector<SelectionChange> sprites;std::vector<int32_t> play_sounds,stop_sounds; };
class SelectionModel {
public:
 size_t add(SelectionItem item);
 const std::vector<SelectionItem>& items()const{return items_;}
 // Native bounds scan selects the last matching normal item unless its +cb
 // stop flag is set. MouseKey/Key/history use the first eligible match.
 std::optional<size_t> hit(int32_t x,int32_t y)const;
 std::optional<SelectionCandidate> candidate(const SelectionInput&)const;
 SelectionNavigationResult navigate(int32_t vk,std::optional<size_t> focused,const SelectionNavigationOptions&)const;
 SelectionUpdate update(const SelectionInput&,std::optional<size_t> focused);
private:
 std::vector<SelectionItem> items_;std::vector<std::array<bool,3>> visibility_;
 std::vector<int> states_;
};
}

namespace yukari {
struct SelectionDecoration {std::optional<SelectionSprite> sprite;std::string text;int32_t x=0,y=0;bool centered=false;};
struct SelectionTimer {int32_t due=0,interval=0;uint32_t branch=0,end=0;bool repeating=false;};
// Stateful parser for declarations which do not allocate selector items as well
// as ordinary items. The host keeps ownership of created sprites and VM waits.
class SelectionDeclarations {
public:
 SelectionModel model;
 SelectionNavigationOptions navigation;
 bool disable_navigation=false;
 std::vector<SelectionDecoration> decorations;
 std::vector<SelectionTimer> timers;
 SelectionRegistrationResult consume(const HostCall&,const SelectionRegistration& options={});
private:
 std::pair<int32_t,int32_t> preceding_text_position_{};
 bool preceding_already_=false;
};
}
