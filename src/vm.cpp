#include "yukari/vm.hpp"
#include "yukari/save.hpp"
#include "yukari/already.hpp"
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace yukari {
namespace {
constexpr size_t max_depth = 256;
constexpr uint32_t max_locals = 65536;
constexpr size_t max_string = 16 * 1024 * 1024;
int32_t integer(const Value& v) {
    if (auto p = std::get_if<int32_t>(&v)) return *p;
    throw std::runtime_error("expected integer (uninitialized or string value)");
}
const std::string& string(const Value& v) {
    if (auto p = std::get_if<std::string>(&v)) return *p;
    throw std::runtime_error("expected string (uninitialized or integer value)");
}
int32_t bits(uint32_t v) {
    int32_t result;
    std::memcpy(&result, &v, sizeof(v));
    return result;
}
void append(std::string& a, const std::string& b) {
    if (b.size() > max_string - a.size()) throw std::runtime_error("VM string limit exceeded");
    a += b;
}
int precedence(char op) {
    switch (op) {
    case '*': case '/': case '%': return 9;
    case '+': case '-': return 8;
    case '=': case '<': case '>': return 7;
    default: throw std::runtime_error("unsupported expression operator");
    }
}
bool case_command(const std::string& name) {
    // These names share handler 0x4386d0 in the original dispatch table.
    for (const char* command : {"Case", "Case2", "MouseCase", "MouseCase2", "MouseCase2Already",
                               "MouseKeyCase", "KeyCase", "KeyTableCase", "IndicateUsualCase",
                               "IndicateCase", "IndicateUsualTimeCase", "TimeInterrupt", "IntervalInterrupt",
                               "AlreadyIndicatePreceding", "IndicateString", "IndicateStringC", "KeyOverPrev",
                               "KeyOverNext", "TabGrurpCheng", "TogetherIndicateCase", "WindowMessageCase",
                               "SelectingKey", "SelectDisable", "CursorMoveTypeIndex", "CursorMoveTypeAuto"})
        if (name == command) return true;
    return false;
}
int32_t binary(char op, int32_t a, int32_t b) {
    switch (op) {
    case '+': return bits(uint32_t(a) + uint32_t(b));
    case '-': return bits(uint32_t(a) - uint32_t(b));
    case '*': return bits(uint32_t(a) * uint32_t(b));
    case '/':
        // 0x43e940 explicitly returns zero on a zero divisor.
        if (!b) return 0;
        [[fallthrough]];
    case '%':
        if (!b) throw std::runtime_error("integer remainder by zero");
        if (a == std::numeric_limits<int32_t>::min() && b == -1)
            throw std::runtime_error("integer division overflow");
        return op == '/' ? a / b : a % b;
    case '=': return a == b;
    case '<': return a < b;
    case '>': return a > b;
    default: throw std::runtime_error("unsupported expression operator");
    }
}
std::string location_text(const VmLocation& l) {
    std::ostringstream out;
    out << l.script << ":word=0x" << std::hex << l.word;
    return out.str();
}
constexpr size_t max_snapshot = 128 * 1024 * 1024;
uint64_t state_hash(const Bytes& data, size_t offset = 0) {
    uint64_t result = 14695981039346656037ULL;
    for (size_t i = offset; i < data.size(); ++i) result = (result ^ data[i]) * 1099511628211ULL;
    return result;
}
uint64_t source_identity(const Script& script) {
    uint64_t hash = 14695981039346656037ULL;
    auto byte = [&](uint8_t n) { hash = (hash ^ n) * 1099511628211ULL; };
    auto number = [&](uint64_t n) { for (unsigned i = 0; i < 8; ++i) byte(uint8_t(n >> (i*8))); };
    auto text = [&](const std::string& value) { number(value.size()); for (unsigned char c : value) byte(c); };
    number(script.max_locals); number(script.encrypted); number(script.elements.size());
    for (const auto& e : script.elements) {
        number(e.type); number(e.a); number(e.b); number(e.c); number(uint32_t(e.value)); text(e.text);
    }
    number(script.instructions.size());
    for (const auto& ins : script.instructions) {
        number(ins.word_offset); number(ins.element); number(ins.arguments.size());
        for (auto argument : ins.arguments) number(argument);
    }
    number(script.data.size()); for (auto b : script.data) byte(b);
    return hash;
}
struct StateWriter {
    Bytes bytes;
    void number(uint64_t n, unsigned size = 4) {
        if (bytes.size() > max_snapshot - size) throw std::runtime_error("VM snapshot exceeds 128 MiB");
        const auto at=bytes.size();bytes.resize(at+size);
        for (unsigned i = 0; i < size; ++i) bytes[at+i]=uint8_t(n >> (i*8));
    }
    void blob(const Bytes& data) {
        if (data.size() > max_snapshot - 4 || bytes.size() > max_snapshot - 4 - data.size())
            throw std::runtime_error("VM snapshot exceeds 128 MiB");
        number(data.size()); bytes.insert(bytes.end(), data.begin(), data.end());
    }
    void text(const std::string& value) {
        if(value.size()>max_snapshot-4||bytes.size()>max_snapshot-4-value.size())throw std::runtime_error("VM snapshot string exceeds 128 MiB");
        number(value.size());bytes.insert(bytes.end(),value.begin(),value.end());
    }
    void value(const Value& value) {
        number(value.index(), 1);
        if (auto n = std::get_if<int32_t>(&value)) number(uint32_t(*n));
        else if (auto s = std::get_if<std::string>(&value)) text(*s);
    }
    void optional(std::optional<uint32_t> value) { number(bool(value), 1); if (value) number(*value); }
};
struct StateReader {
    const Bytes& bytes;
    size_t position = 0;
    uint64_t number(unsigned size = 4) {
        require_range(bytes.size(), position, size, "VM snapshot field");
        uint64_t result = 0;
        for (unsigned i = 0; i < size; ++i) result |= uint64_t(bytes[position++]) << (i*8);
        return result;
    }
    bool boolean() {
        const auto n = number(1);
        if (n > 1) throw std::runtime_error("invalid VM snapshot boolean");
        return n != 0;
    }
    uint32_t count(uint32_t maximum) {
        const auto n = number();
        if (n > maximum) throw std::runtime_error("VM snapshot collection exceeds limit");
        return uint32_t(n);
    }
    Bytes blob() {
        const auto size = count(uint32_t(max_snapshot));
        require_range(bytes.size(), position, size, "VM snapshot blob");
        Bytes result(bytes.begin() + position, bytes.begin() + position + size);
        position += size; return result;
    }
    std::string text() {
        auto data = blob();
        if (data.size() > max_string) throw std::runtime_error("VM snapshot string exceeds limit");
        return {data.begin(), data.end()};
    }
    Value value() {
        switch (number(1)) {
        case 0: return {};
        case 1: return bits(uint32_t(number()));
        case 2: return text();
        default: throw std::runtime_error("invalid VM snapshot value tag");
        }
    }
    std::optional<uint32_t> optional() { return boolean() ? std::optional<uint32_t>(uint32_t(number())) : std::nullopt; }
};
}
std::string format_value(const Value& value) {
    if (auto p = std::get_if<int32_t>(&value)) return std::to_string(*p);
    if (auto p = std::get_if<std::string>(&value)) {
        std::ostringstream out;
        out << '"';
        for (unsigned char c : *p) {
            if (c == '"' || c == '\\') out << '\\' << c;
            else if (c < 32 || c == 127) out << "\\x" << std::hex << std::setw(2) << std::setfill('0') << unsigned(c) << std::dec;
            else out << c;
        }
        out << '"';
        return out.str();
    }
    return "none";
}

struct Vm::Impl {
    struct Frame {
        std::string name;
        Script script;
        std::unordered_map<uint32_t, size_t> words;
        std::unordered_map<uint32_t, int32_t> constants;
        std::vector<Value> locals;
        std::optional<uint32_t> target;
        std::optional<uint32_t> suspended_call;
        std::optional<uint32_t> collect_end;
        uint32_t pc = 0, end = 0;
        Value returned{};
        uint64_t source_hash{};
        Bytes native_header;
        std::optional<uint32_t> native_next;
        bool subcall=false;
        uint32_t skip_mode=0;bool submenu=false;
        int32_t select_id=0;bool select_active=false;
    };
    struct Injection {
        uint32_t parent_depth;
        bool waiting, selection_ready;
        uint32_t wait_next, selection_begin, selection_end;
    };
    struct Interrupt { uint32_t depth, pc, end; std::optional<uint32_t> target; uint32_t wait_next, begin, selection_end; };
    std::vector<Interrupt> interrupts;
    Loader loader;
    Host host;
    Trace trace;
    std::vector<std::unique_ptr<Frame>> frames;
    std::vector<Injection> injections;
    Bytes native_header;
    std::unordered_map<int32_t, int32_t> flags, global_flags;
    std::unordered_map<int32_t, std::string> strings, global_strings;
    std::unordered_map<std::string, std::string> special_strings;
    Value result{};
    AlreadyData already;
    bool current_already=false;
    int32_t script_state=0;
    uint64_t count = 0;
    bool waiting = false, running = false;
    uint32_t wait_next = 0;
    bool selection_ready = false;
    uint32_t selection_begin = 0, selection_end = 0;

    explicit Impl(Loader l) : loader(std::move(l)) {
        if (!loader) throw std::runtime_error("VM requires a script loader");
    }
    static void check_bank(const std::string& bank, int32_t index) {
        const bool global = bank == "GlobalFlag" || bank == "GlobalString";
        if (!global && bank != "Flag" && bank != "String") throw std::runtime_error("unknown variable bank: " + bank);
        if (index < 0 || index >= (global ? 10000 : 65536)) throw std::runtime_error("variable index out of range: " + bank);
    }
    Value variable(const std::string& bank, int32_t index) const {
        check_bank(bank, index);
        if (bank == "Flag" || bank == "GlobalFlag") {
            const auto& values = bank == "Flag" ? flags : global_flags;
            auto it = values.find(index);
            return it == values.end() ? int32_t(0) : it->second;
        }
        const auto& values = bank == "String" ? strings : global_strings;
        auto it = values.find(index);
        return it == values.end() ? std::string{} : it->second;
    }
    void set_variable(const std::string& bank, int32_t index, const Value& value) {
        check_bank(bank, index);
        if (bank == "Flag" || bank == "GlobalFlag") {
            (bank == "Flag" ? flags : global_flags)[index] = integer(value);
        } else {
            const auto& s = string(value);
            if (s.size() > max_string) throw std::runtime_error("VM string limit exceeded");
            (bank == "String" ? strings : global_strings)[index] = s;
        }
    }
    std::unique_ptr<Frame> load(const std::string& name, const Script* embedded = nullptr) {
        if (frames.size() >= max_depth) throw std::runtime_error("VM script depth limit exceeded");
        auto f = std::make_unique<Frame>();
        if(!frames.empty()){f->subcall=frames.back()->subcall;f->skip_mode=frames.back()->skip_mode;f->submenu=frames.back()->submenu;}
        f->name = name;
        f->script = loader(name);
        f->source_hash = source_identity(f->script);
        if (embedded) f->script = *embedded;
        if (f->script.max_locals > max_locals) throw std::runtime_error("VM local count limit exceeded");
        f->locals.resize(f->script.max_locals);
        uint64_t next = 0;
        for (size_t i = 0; i < f->script.instructions.size(); ++i) {
            const auto& ins = f->script.instructions[i];
            if (ins.word_offset != next || ins.element >= f->script.elements.size())
                throw std::runtime_error("invalid VM instruction layout");
            const auto type = f->script.elements[ins.element].type;
            if (type != 0 && !ins.arguments.empty()) throw std::runtime_error("arguments on non-function element");
            if (ins.arguments.size() > 128) throw std::runtime_error("VM parameter count exceeds original limit 128");
            for (auto arg : ins.arguments) if (arg >= f->script.elements.size()) throw std::runtime_error("VM argument index out of range");
            f->words.emplace(ins.word_offset, i);
            next += type == 0 ? ins.arguments.size() + 2 : 1;
            if (next > std::numeric_limits<uint32_t>::max()) throw std::runtime_error("VM code length overflow");
        }
        f->end = static_cast<uint32_t>(next);
        already.bind(f->name,f->end);
        for (const auto& e : f->script.elements) {
            if (e.type == 4) {
                const auto item = f->constants.emplace(e.b, e.value);
                if (!item.second && item.first->second != e.value) throw std::runtime_error("inconsistent integer data alias");
            }
            if (e.type == 10 && e.b >= f->locals.size()) throw std::runtime_error("local index exceeds header allocation");
        }
        return f;
    }
    const Instruction& at(const Frame& f, uint32_t word) const {
        auto it = f.words.find(word);
        if (it == f.words.end()) throw std::runtime_error("jump is not an instruction boundary");
        return f.script.instructions[it->second];
    }
    int32_t cell(const Frame& f, uint32_t offset, int32_t fallback) const {
        return f.script.data.empty() ? fallback : bits(u32(f.script.data, offset));
    }
    uint32_t link(const Frame& f, const Element& e, bool after = false) const {
        const auto target = cell(f, e.b, e.value);
        if (target < 0) throw std::runtime_error("unresolved negative control link");
        const uint64_t word = uint64_t(target) + (after ? 1 : 0);
        if (word > f.end) throw std::runtime_error("control link outside script");
        if (word < f.end) at(f, static_cast<uint32_t>(word));
        return static_cast<uint32_t>(word);
    }
    uint32_t label(const Frame& f, const std::string& name) const {
        for (const auto& e : f.script.elements) if (e.type == 1 && e.text == name) return link(f, e);
        throw std::runtime_error("script label not found: " + name);
    }
    Value value(Frame& f, uint32_t index) {
        const auto& e = f.script.elements.at(index);
        switch (e.type) {
        case 1: return e.text;
        case 4: return cell(f, e.b, f.constants.at(e.b));
        case 5: return e.text;
        case 7: return special_strings[e.text];
        case 8: case 9: return variable(e.text, cell(f, e.c, e.value));
        case 10:
            if (e.b >= f.locals.size()) throw std::runtime_error("local index out of range");
            if (std::holds_alternative<std::monostate>(f.locals[e.b])) throw std::runtime_error("read of uninitialized local $" + std::to_string(e.b));
            return f.locals[e.b];
        default: throw std::runtime_error("unsupported VM argument type " + std::to_string(e.type));
        }
    }
    void store(Frame& f, uint32_t index, const Value& v) {
        if (std::holds_alternative<std::monostate>(v)) return;
        if (auto s = std::get_if<std::string>(&v); s && s->size() > max_string)
            throw std::runtime_error("VM string limit exceeded");
        const auto& e = f.script.elements.at(index);
        switch (e.type) {
        case 4: {
            const auto n = integer(v);
            f.constants.at(e.b) = n;
            if (!f.script.data.empty()) {
                require_range(f.script.data.size(), e.b, 4, "integer assignment");
                for (unsigned i = 0; i < 4; ++i) f.script.data[e.b+i] = uint8_t(uint32_t(n) >> (i*8));
            }
            break;
        }
        case 7: special_strings[e.text] = string(v); break;
        case 8: case 9: set_variable(e.text, cell(f, e.c, e.value), v); break;
        case 10:
            if (e.b >= f.locals.size()) throw std::runtime_error("local index out of range");
            f.locals[e.b] = v; break;
        default: throw std::runtime_error("unsupported VM assignment target");
        }
    }
    void finish_call(Frame& f, uint32_t next, const Value& v) {
        if (f.target) store(f, *f.target, v);
        f.target.reset();
        f.pc = next;
    }
    Value expression(Frame& f, const Instruction& ins) {
        const auto& args = ins.arguments;
        if (args.empty()) return {};
        const Value first = value(f, args.front());
        // String expression mode is selected by the first argument's runtime type.
        if (std::holds_alternative<std::string>(first) && f.script.elements[args.front()].type != 1) {
            std::string sides[2];
            size_t side = 0;
            bool equality = false;
            for (auto a : args) {
                const auto& e = f.script.elements[a];
                if (e.type == 1) {
                    if (e.text == "=") { equality = true; ++side; }
                } else {
                    Value v = value(f, a);
                    if (std::holds_alternative<std::string>(v) && side < 2) append(sides[side], string(v));
                }
            }
            if (equality) return int32_t(sides[0] == sides[1]);
            // The original returns no object for a multi-argument empty concatenation.
            if (args.size() > 1 && sides[0].empty()) return {};
            return sides[0];
        }
        std::vector<int32_t> values;
        std::vector<char> operators;
        auto reduce = [&] {
            if (values.size() < 2 || operators.empty()) throw std::runtime_error("malformed integer expression");
            const auto b = values.back(); values.pop_back();
            const auto a = values.back(); values.pop_back();
            const auto op = operators.back(); operators.pop_back();
            values.push_back(binary(op, a, b));
        };
        bool operand = true;
        for (auto a : args) {
            const auto& e = f.script.elements[a];
            if (e.type == 1) {
                if (e.text.size() != 1) throw std::runtime_error("unsupported expression control token: " + e.text);
                const char op = e.text.front();
                const int prec = precedence(op);
                // Original postfix builder has an initial zero operand.
                if (operand && values.empty()) values.push_back(0);
                else if (operand) throw std::runtime_error("adjacent expression operators");
                while (!operators.empty() && precedence(operators.back()) >= prec) reduce();
                operators.push_back(op);
                operand = true;
            } else {
                if (!operand) throw std::runtime_error("adjacent expression values");
                values.push_back(integer(value(f, a)));
                operand = false;
            }
        }
        if (operand) throw std::runtime_error("expression ends with an operator");
        while (!operators.empty()) reduce();
        if (values.size() != 1) throw std::runtime_error("malformed integer expression");
        return values.back();
    }
    std::string diagnostic(const std::string& reason) const {
        std::ostringstream out;
        out << reason;
        for (auto i = frames.rbegin(); i != frames.rend(); ++i) {
            out << "\n  at " << location_text({(*i)->name, (*i)->suspended_call.value_or((*i)->pc)});
        }
        return out.str();
    }
    HostCall host_call(Frame& f, const Instruction& ins, uint32_t next) {
        HostCall call{f.script.elements[ins.element].text, {f.name, f.pc}, {}, {}, {}};
        call.subcall=f.subcall;
        call.arguments.reserve(ins.arguments.size());
        for (auto a : ins.arguments) call.arguments.push_back(value(f, a));
        if (next < f.end) {
            const auto& opening = f.script.elements[at(f, next).element];
            if (opening.type == 1 && opening.text == "{") {
                call.block_begin = next;
                call.block_end = link(f, opening);
            }
        }
        return call;
    }
    void script_call(Frame& f,const Instruction& ins,uint32_t next,bool jump,bool subcall) {
        if(ins.arguments.empty()||ins.arguments.size()>2)throw std::runtime_error("ScriptCall/ScriptJump/ScriptSubCall requires one or two arguments");
        const auto& arg=f.script.elements[ins.arguments[0]];
        if(ins.arguments.size()==1&&arg.type==1&&jump){finish_call(f,link(f,arg),{});return;}
        const std::string name=arg.type==1?f.name:string(value(f,ins.arguments[0]));
        auto child=load(name);
        if(arg.type==1)child->pc=label(*child,arg.text);
        if(ins.arguments.size()==2){const auto& target=f.script.elements[ins.arguments[1]];if(target.type!=1)throw std::runtime_error("second script call argument must be a label");child->pc=label(*child,target.text);}
        if(subcall)child->subcall=true;
        f.suspended_call=f.pc;finish_call(f,jump?f.end:next,{});frames.push_back(std::move(child));
    }
    void step() {
        Frame& f = *frames.back();
        const auto& ins = at(f, f.pc);
        const auto& e = f.script.elements.at(ins.element);
        current_already=already.mark(f.name,f.pc,f.end);
        f.native_next.reset();
        if (trace) trace({f.name, f.pc}, e.type == 0 ? e.text : (e.type == 1 ? "label " + e.text : "target"));
        ++count;
        if (e.type != 0) {
            if (e.type == 1) {
                f.target.reset();
                uint32_t next = f.pc + 1;
                if (e.text == "else" && next < f.end) {
                    const auto& opening = f.script.elements[at(f, next).element];
                    if (opening.type == 1 && opening.text == "{") next = link(f, opening, true);
                }
                f.pc = next;
            } else {
                if (e.type != 4 && e.type != 7 && e.type != 8 && e.type != 9 && e.type != 10)
                    throw std::runtime_error("unsupported VM target type");
                f.target = ins.element;
                ++f.pc;
            }
            return;
        }
        const uint32_t next = f.pc + static_cast<uint32_t>(ins.arguments.size()) + 2;
        if(e.text=="NowScriptStateGet"){finish_call(f,next,script_state);return;}
        if(e.text=="SelectID"){
            if(!ins.arguments.empty()){f.select_id=integer(value(f,ins.arguments[0]));f.select_active=true;}
            finish_call(f,next,int32_t(f.select_active?f.select_id:-1));return;
        }
        if (e.text == "=") {
            // =() selects a target for the following function; it is not an assignment.
            if (ins.arguments.empty()) f.pc = next;
            else finish_call(f, next, expression(f, ins));
            return;
        }
        if (e.text == "if" || e.text == "If" || case_command(e.text)) {
            const bool is_case = case_command(e.text);
            if (!is_case && ins.arguments.empty()) throw std::runtime_error("if requires a condition");
            uint32_t destination = next;
            if (is_case && f.collect_end && host) host(host_call(f, ins, next));
            if ((is_case || !integer(value(f, ins.arguments[0]))) && next < f.end) {
                const auto& opening = f.script.elements[at(f, next).element];
                if (opening.type == 1 && opening.text == "{") destination = link(f, opening);
            }
            finish_call(f, destination, {});
            return;
        }
        if (e.text == "ScriptCall" || e.text == "ScriptJump" || e.text == "ScriptSubCall") {
            const bool subcall=e.text=="ScriptSubCall";
            if(subcall) {
                // 0x446410 writes savedata.--- BEFORE calling ScriptCall and
                // setting the new frame's +0xc9. A deferred host checkpoint
                // must finish outside run() before resume() pushes the child.
                auto response=host?host(host_call(f,ins,next)):std::nullopt;
                if(!response)throw std::runtime_error("ScriptSubCall requires a host checkpoint service");
                if(response->collect_block||response->skip_block)throw std::runtime_error("invalid ScriptSubCall checkpoint response");
                if(response->waiting){waiting=true;wait_next=next;return;}
            }
            script_call(f,ins,next,e.text=="ScriptJump",subcall);
            return;
        }
        if (e.text == "Return") {
            if (ins.arguments.size() > 1) throw std::runtime_error("Return accepts at most one integer");
            Value v;
            if (!ins.arguments.empty()) v = integer(value(f, ins.arguments[0]));
            f.returned = v;
            finish_call(f, f.end, v);
            return;
        }
        HostCall call = host_call(f, ins, next);
        auto response = host ? host(call) : std::nullopt;
        if (!response) throw std::runtime_error("unimplemented engine function: " + e.text);
        if (response->collect_block) {
            if (!call.block_begin || !call.block_end || response->waiting || response->skip_block) throw std::runtime_error("invalid host selection block");
            f.collect_end = *call.block_end;
            selection_begin = *call.block_begin;
        }
        const auto destination=response->skip_block&&call.block_end?*call.block_end:next;
        if (response->waiting) {
            waiting = true;
            wait_next = destination;
        } else finish_call(f, destination, response->value);
    }
};
Vm::Vm(Loader loader) : impl_(std::make_unique<Impl>(std::move(loader))) {}
Vm::~Vm() = default;
void Vm::set_host(Host host) { impl_->host = std::move(host); }
void Vm::set_trace(Trace trace) { impl_->trace = std::move(trace); }
void Vm::start(const std::string& entry) {
    if (impl_->running || impl_->waiting || !impl_->frames.empty()) throw std::runtime_error("VM is already active");
    try { impl_->frames.push_back(impl_->load(entry)); }
    catch (const std::exception& e) { throw std::runtime_error("loading script " + entry + ": " + e.what()); }
    impl_->result = {};
    impl_->count = 0;
}
void Vm::reset(const std::string& entry) {
    if(impl_->running)throw std::runtime_error("cannot reset VM during run");
    auto next=std::make_unique<Impl>(impl_->loader);
    next->host=impl_->host;next->trace=impl_->trace;next->already=impl_->already;
    next->script_state=impl_->script_state;
    next->global_flags=impl_->global_flags;next->global_strings=impl_->global_strings;
    try { next->frames.push_back(next->load(entry)); }
    catch(const std::exception& e){throw std::runtime_error("loading script "+entry+": "+e.what());}
    impl_=std::move(next);
}
void Vm::call(const std::string& entry,bool subcall) {
    if (impl_->running || impl_->frames.empty()) throw std::runtime_error("VM call requires a paused active context");
    auto child = impl_->load(entry);if(subcall)child->subcall=true;
    impl_->injections.push_back({uint32_t(impl_->frames.size()), impl_->waiting, impl_->selection_ready,
                                impl_->wait_next, impl_->selection_begin, impl_->selection_end});
    impl_->frames.push_back(std::move(child));
    impl_->waiting = impl_->selection_ready = false;
}
int32_t Vm::script_state()const{return impl_->script_state;}
void Vm::set_script_state(int32_t state){impl_->script_state=state;}
bool Vm::in_call() const { return !impl_->injections.empty(); }
bool Vm::in_subcall() const { return !impl_->frames.empty()&&impl_->frames.back()->subcall; }
bool Vm::pending_subcall() const {
    if(!impl_->waiting||impl_->selection_ready||impl_->frames.empty())return false;
    const auto& f=*impl_->frames.back();if(f.pc>=f.end)return false;
    const auto& e=f.script.elements[impl_->at(f,f.pc).element];return e.type==0&&e.text=="ScriptSubCall";
}
uint32_t Vm::current_frame_skip_mode()const{return impl_->frames.empty()?0:impl_->frames.back()->skip_mode;}
void Vm::set_current_frame_skip_mode(uint32_t mode){if(impl_->frames.empty())throw std::runtime_error("VM has no current frame");impl_->frames.back()->skip_mode=mode;}
bool Vm::submenu_enabled()const{return !impl_->frames.empty()&&impl_->frames.back()->submenu;}
void Vm::set_submenu_enabled(bool enabled){if(impl_->frames.empty())throw std::runtime_error("VM has no current frame");impl_->frames.back()->submenu=enabled;}
std::optional<int32_t> Vm::current_frame_select_id()const{if(impl_->frames.empty()||!impl_->frames.back()->select_active)return std::nullopt;return impl_->frames.back()->select_id;}
void Vm::set_current_frame_select_id(int32_t id){if(impl_->frames.empty())throw std::runtime_error("VM has no current frame");impl_->frames.back()->select_id=id;impl_->frames.back()->select_active=true;}
VmStatus Vm::run(uint64_t limit) {
    if (impl_->running) throw std::runtime_error("VM run is not reentrant");
    if (impl_->waiting) return VmStatus::Waiting;
    struct Guard { bool& value; ~Guard() { value = false; } } guard{impl_->running};
    impl_->running = true;
    uint64_t executed = 0;
    try {
        while (!impl_->frames.empty()) {
            auto& f = *impl_->frames.back();
            if (!impl_->interrupts.empty() && impl_->interrupts.back().depth == impl_->frames.size() && f.pc == impl_->interrupts.back().end) {
                const auto pause=impl_->interrupts.back();impl_->interrupts.pop_back();
                f.pc=pause.pc;f.target=pause.target;
                impl_->wait_next=pause.wait_next;impl_->selection_begin=pause.begin;impl_->selection_end=pause.selection_end;
                impl_->waiting=impl_->selection_ready=true;return VmStatus::Waiting;
            }
            if (f.pc == f.end) {
                if (impl_->frames.size() == 1) impl_->result = f.returned;
                impl_->frames.pop_back();
                if (!impl_->frames.empty()) impl_->frames.back()->suspended_call.reset();
                if (!impl_->injections.empty() && impl_->injections.back().parent_depth == impl_->frames.size()) {
                    const auto wait = impl_->injections.back(); impl_->injections.pop_back();
                    impl_->waiting = wait.waiting; impl_->selection_ready = wait.selection_ready;
                    impl_->wait_next = wait.wait_next; impl_->selection_begin = wait.selection_begin;
                    impl_->selection_end = wait.selection_end;
                    if (impl_->waiting) return VmStatus::Waiting;
                }
                continue;
            }
            if (f.collect_end && f.pc == *f.collect_end) {
                impl_->waiting = impl_->selection_ready = true;
                impl_->selection_end = f.pc;
                impl_->wait_next = f.pc + 1;
                f.collect_end.reset();
                return VmStatus::Waiting;
            }
            if (executed == limit) return VmStatus::StepLimit;
            impl_->step();
            ++executed;
            if (impl_->waiting) return VmStatus::Waiting;
        }
    } catch (const std::exception& e) { throw std::runtime_error(impl_->diagnostic(e.what())); }
    return VmStatus::Halted;
}
void Vm::resume(Value result) {
    if (!impl_->waiting || impl_->frames.empty()) throw std::runtime_error("VM has no suspended host call");
    try {
        auto& frame=*impl_->frames.back();
        const Instruction* ins=frame.pc<frame.end?&impl_->at(frame,frame.pc):nullptr;
        if(!impl_->selection_ready&&ins&&frame.script.elements[ins->element].type==0&&frame.script.elements[ins->element].text=="ScriptSubCall")
            impl_->script_call(frame,*ins,impl_->wait_next,false,true);
        else impl_->finish_call(frame,impl_->wait_next,result);
        impl_->waiting = false;
        impl_->selection_ready = false;
    } catch (const std::exception& e) { throw std::runtime_error(impl_->diagnostic(e.what())); }
}
void Vm::choose(uint32_t word) {
    if (!impl_->waiting || !impl_->selection_ready || impl_->frames.empty()) throw std::runtime_error("VM has no pending selection");
    if (word <= impl_->selection_begin || word > impl_->selection_end) throw std::runtime_error("selection branch outside block");
    auto& f = *impl_->frames.back();
    impl_->at(f, word);
    f.pc = word;
    f.target.reset();
    impl_->waiting = impl_->selection_ready = false;
}
void Vm::interrupt_selection(uint32_t word,uint32_t end) {
    if(impl_->running||!impl_->waiting||!impl_->selection_ready||impl_->frames.empty())throw std::runtime_error("VM has no paused selection to interrupt");
    auto& f=*impl_->frames.back();
    if(word<=impl_->selection_begin||word>=end||end>=impl_->selection_end)throw std::runtime_error("selection interrupt outside block");
    impl_->at(f,word);const auto& closing=f.script.elements[impl_->at(f,end).element];
    if(closing.type!=1||closing.text!="}")throw std::runtime_error("selection interrupt requires closing block label");
    impl_->interrupts.push_back({uint32_t(impl_->frames.size()),f.pc,end,f.target,impl_->wait_next,impl_->selection_begin,impl_->selection_end});
    f.pc=word;f.target.reset();impl_->waiting=impl_->selection_ready=false;
}
Value Vm::variable(const std::string& bank, int32_t index) const { return impl_->variable(bank, index); }
void Vm::set_variable(const std::string& bank, int32_t index, Value value) { impl_->set_variable(bank, index, value); }
void Vm::clear_variables(){impl_->flags.clear();impl_->strings.clear();}

bool Vm::current_already() const { return impl_->current_already; }
bool Vm::is_already(const std::string& name,uint32_t word) const { return impl_->already.read(name,word); }
void Vm::load_already(const Bytes& bytes,const char* encoding) {
    if(impl_->running)throw std::runtime_error("cannot load Already.dat during VM execution");
    auto data=decode_already(bytes,encoding);
    for(const auto& frame:impl_->frames)data.bind(frame->name,frame->end);
    impl_->already=std::move(data);impl_->current_already=false;
}
Bytes Vm::save_already(const char* encoding) const { return encode_already(impl_->already,encoding); }
const Value& Vm::last_result() const { return impl_->result; }
uint64_t Vm::steps() const { return impl_->count; }
Bytes Vm::snapshot() const {
    if (impl_->running) throw std::runtime_error("cannot snapshot VM during run");
    StateWriter payload;
    // Reserve once for the common snapshot shape; the serialized format and
    // all per-field limits remain unchanged. Cap speculation at 16 MiB.
    uint64_t hint=1024;
    for(const auto& frame:impl_->frames){const auto& f=*frame;
        hint+=f.script.data.size()+f.script.elements.size()*32+f.script.instructions.size()*24+f.locals.size()*16;
        for(const auto& e:f.script.elements)hint+=e.text.size();
    }
    payload.bytes.reserve(size_t(std::min<uint64_t>(hint,16*1024*1024)));
    payload.number(impl_->count, 8); payload.value(impl_->result); payload.blob(impl_->native_header); payload.number(uint32_t(impl_->script_state));
    payload.number(impl_->waiting, 1); payload.number(impl_->wait_next);
    payload.number(impl_->selection_ready, 1); payload.number(impl_->selection_begin); payload.number(impl_->selection_end);
    auto integers = [&](const auto& bank) {
        std::vector<std::pair<int32_t,int32_t>> ordered(bank.begin(),bank.end());
        std::sort(ordered.begin(),ordered.end());
        payload.number(ordered.size());
        for (const auto& item : ordered) { payload.number(uint32_t(item.first)); payload.number(uint32_t(item.second)); }
    };
    auto strings = [&](const auto& bank) {
        std::vector<const typename std::decay_t<decltype(bank)>::value_type*> ordered;
        ordered.reserve(bank.size());for(const auto& item:bank)ordered.push_back(&item);
        std::sort(ordered.begin(),ordered.end(),[](auto a,auto b){return a->first<b->first;});
        payload.number(ordered.size());
        for (const auto* item : ordered) { payload.number(uint32_t(item->first)); payload.text(item->second); }
    };
    integers(impl_->flags); integers(impl_->global_flags); strings(impl_->strings); strings(impl_->global_strings);
    const std::map<std::string, std::string> special(impl_->special_strings.begin(), impl_->special_strings.end());
    payload.number(special.size());
    for (const auto& item : special) { payload.text(item.first); payload.text(item.second); }
    payload.number(impl_->frames.size());
    for (const auto& frame : impl_->frames) {
        const auto& f = *frame;
        payload.text(f.name); payload.number(f.source_hash, 8);
        payload.number(f.script.max_locals); payload.number(f.script.encrypted, 1);
        payload.number(f.script.elements.size());
        for (const auto& e : f.script.elements) {
            payload.number(e.type); payload.number(e.a); payload.number(e.b); payload.number(e.c);
            payload.text(e.text); payload.number(uint32_t(e.value));
        }
        payload.number(f.script.instructions.size());
        for (const auto& ins : f.script.instructions) {
            payload.number(ins.word_offset); payload.number(ins.element); payload.number(ins.arguments.size());
            for (auto arg : ins.arguments) payload.number(arg);
        }
        payload.blob(f.script.data);
        payload.blob(f.native_header);
        payload.optional(f.native_next);
        payload.number(f.subcall,1);payload.number(f.skip_mode);payload.number(f.submenu,1);payload.number(uint32_t(f.select_id));payload.number(f.select_active,1);
        payload.number(f.pc); payload.optional(f.target); payload.optional(f.suspended_call); payload.optional(f.collect_end);
        payload.value(f.returned);
        payload.number(f.locals.size()); for (const auto& value : f.locals) payload.value(value);
        std::vector<std::pair<uint32_t,int32_t>> constants(f.constants.begin(),f.constants.end());
        std::sort(constants.begin(),constants.end());
        payload.number(constants.size());
        for (const auto& item : constants) { payload.number(item.first); payload.number(uint32_t(item.second)); }
    }
    payload.number(impl_->injections.size());
    for (const auto& pause : impl_->injections) {
        payload.number(pause.parent_depth); payload.number(pause.waiting, 1); payload.number(pause.selection_ready, 1);
        payload.number(pause.wait_next); payload.number(pause.selection_begin); payload.number(pause.selection_end);
    }
    payload.number(impl_->interrupts.size());
    for(const auto& pause:impl_->interrupts) {
        payload.number(pause.depth);payload.number(pause.pc);payload.number(pause.end);payload.optional(pause.target);
        payload.number(pause.wait_next);payload.number(pause.begin);payload.number(pause.selection_end);
    }
    StateWriter output;
    output.bytes = {'Y','U','K','A','R','I','V','M'};
    output.number(9); output.number(payload.bytes.size()); output.number(state_hash(payload.bytes), 8);
    if (payload.bytes.size() > max_snapshot - output.bytes.size()) throw std::runtime_error("VM snapshot exceeds 128 MiB");
    output.bytes.insert(output.bytes.end(), payload.bytes.begin(), payload.bytes.end());
    return output.bytes;
}
Bytes Vm::snapshot_after_subcall() const {
    if(!pending_subcall())throw std::runtime_error("no suspended ScriptSubCall checkpoint");
    Vm copy(impl_->loader);copy.restore(snapshot());
    copy.impl_->finish_call(*copy.impl_->frames.back(),copy.impl_->wait_next,{});
    copy.impl_->waiting=false;copy.impl_->selection_ready=false;
    return copy.snapshot();
}
void Vm::restore(const Bytes& bytes) {
    if (impl_->running) throw std::runtime_error("cannot restore VM during run");
    if (bytes.size() < 24 || bytes.size() > max_snapshot || std::memcmp(bytes.data(), "YUKARIVM", 8))
        throw std::runtime_error("invalid Yukari VM snapshot");
    StateReader input{bytes, 8};
    const auto version = input.number();
    if ((version < 1 || version > 9) || input.number() != bytes.size() - 24) throw std::runtime_error("unsupported VM snapshot version/length");
    if (input.number(8) != state_hash(bytes, 24)) throw std::runtime_error("VM snapshot checksum mismatch");
    auto candidate = std::make_unique<Impl>(impl_->loader);
    candidate->host = impl_->host; candidate->trace = impl_->trace;
    candidate->already=impl_->already;candidate->script_state=impl_->script_state;
    candidate->count = input.number(8); candidate->result = input.value();
    if (version >= 3) {
        candidate->native_header = input.blob();
        if (!candidate->native_header.empty() && candidate->native_header.size() != 0x160)
            throw std::runtime_error("invalid native state header in snapshot");
    }
    if(version>=9)candidate->script_state=bits(uint32_t(input.number()));
    candidate->waiting = input.boolean(); candidate->wait_next = uint32_t(input.number());
    candidate->selection_ready = input.boolean(); candidate->selection_begin = uint32_t(input.number());
    candidate->selection_end = uint32_t(input.number());
    for (const auto& bank : {"Flag", "GlobalFlag", "String", "GlobalString"}) {
        const bool text_bank = std::string(bank).find("String") != std::string::npos;
        const auto size = input.count(65536);
        std::vector<int32_t> indices;
        for (uint32_t i = 0; i < size; ++i) {
            const auto index = bits(uint32_t(input.number()));
            if (!indices.empty() && index <= indices.back()) throw std::runtime_error("unordered VM variable bank");
            indices.push_back(index);
            Value value = text_bank ? Value(input.text()) : Value(bits(uint32_t(input.number())));
            candidate->set_variable(bank, index, value);
        }
    }
    for (uint32_t i = 0, count = input.count(65536); i < count; ++i) {
        const auto name = input.text(), value = input.text();
        if (!candidate->special_strings.emplace(name, value).second) throw std::runtime_error("duplicate special VM string");
    }
    auto boundary = [&](const Impl::Frame& f, uint32_t word) {
        if (word != f.end) candidate->at(f, word);
    };
    for (uint32_t i = 0, count = input.count(uint32_t(max_depth)); i < count; ++i) {
        const auto name = input.text(); const auto expected_source = input.number(8);
        Script embedded;
        if (version >= 2) {
            embedded.max_locals = input.count(max_locals); embedded.encrypted = input.boolean();
            for (uint32_t e = 0, count = input.count(1000000); e < count; ++e) {
                Element element{};
                element.type = uint32_t(input.number()); element.a = uint32_t(input.number());
                element.b = uint32_t(input.number()); element.c = uint32_t(input.number());
                element.text = input.text(); element.value = bits(uint32_t(input.number()));
                embedded.elements.push_back(std::move(element));
            }
            for (uint32_t ins = 0, count = input.count(4000000); ins < count; ++ins) {
                Instruction instruction{};
                instruction.word_offset = uint32_t(input.number()); instruction.element = uint32_t(input.number());
                for (uint32_t arg = 0, count = input.count(128); arg < count; ++arg)
                    instruction.arguments.push_back(uint32_t(input.number()));
                embedded.instructions.push_back(std::move(instruction));
            }
            embedded.data = input.blob();
        }
        auto frame = candidate->load(name, version >= 2 ? &embedded : nullptr);
        if (expected_source != frame->source_hash) throw std::runtime_error("VM snapshot source script changed: " + frame->name);
        if (version == 1) {
            auto data = input.blob();
            if (data.size() != frame->script.data.size()) throw std::runtime_error("VM snapshot data section length changed");
            frame->script.data = std::move(data);
        }
        if (version >= 3) {
            frame->native_header = input.blob();
            if (!frame->native_header.empty() && frame->native_header.size() != 0x50)
                throw std::runtime_error("invalid native frame header in snapshot");
        }
        if (version >= 4) frame->native_next = input.optional();
        if (version >= 5) frame->subcall=input.boolean();
        if(version>=7){frame->skip_mode=uint32_t(input.number());frame->submenu=input.boolean();}else if(frame->native_header.size()==0x50){frame->skip_mode=u32(frame->native_header,0x38);frame->submenu=u32(frame->native_header,0x3c)!=0;}
        if(version>=8){frame->select_id=bits(uint32_t(input.number()));frame->select_active=input.boolean();}
        else if(frame->native_header.size()==0x50){frame->select_id=bits(u32(frame->native_header,0x4c));frame->select_active=u32(frame->native_header,0x48)!=0;}
        frame->pc = uint32_t(input.number()); boundary(*frame, frame->pc);
        if (frame->native_next) boundary(*frame,*frame->native_next);
        frame->target = input.optional(); frame->suspended_call = input.optional(); frame->collect_end = input.optional();
        if (frame->target) {
            if (*frame->target >= frame->script.elements.size()) throw std::runtime_error("VM snapshot target out of range");
            const auto type = frame->script.elements[*frame->target].type;
            if (type != 4 && type != 7 && type != 8 && type != 9 && type != 10)
                throw std::runtime_error("VM snapshot target is not assignable");
        }
        if (frame->suspended_call) candidate->at(*frame, *frame->suspended_call);
        if (frame->collect_end) candidate->at(*frame, *frame->collect_end);
        frame->returned = input.value();
        const auto locals = input.count(max_locals);
        if (locals != frame->locals.size()) throw std::runtime_error("VM snapshot local count changed");
        for (auto& value : frame->locals) value = input.value();
        const auto constants = input.count(1000000);
        if (constants != frame->constants.size()) throw std::runtime_error("VM snapshot constant count changed");
        std::map<uint32_t, int32_t> values;
        for (uint32_t n = 0; n < constants; ++n) {
            const auto offset = uint32_t(input.number()); const auto value = bits(uint32_t(input.number()));
            if (!frame->constants.count(offset) || !values.emplace(offset, value).second)
                throw std::runtime_error("invalid VM snapshot constant offset");
        }
        for (const auto& item : values) frame->constants[item.first] = item.second;
        candidate->frames.push_back(std::move(frame));
    }
    for (uint32_t i = 0, count = input.count(uint32_t(max_depth)); i < count; ++i) {
        Impl::Injection pause;
        pause.parent_depth = uint32_t(input.number()); pause.waiting = input.boolean(); pause.selection_ready = input.boolean();
        pause.wait_next = uint32_t(input.number()); pause.selection_begin = uint32_t(input.number()); pause.selection_end = uint32_t(input.number());
        if (!pause.parent_depth || pause.parent_depth >= candidate->frames.size() ||
            (!candidate->injections.empty() && pause.parent_depth <= candidate->injections.back().parent_depth))
            throw std::runtime_error("invalid injected VM context depth");
        const auto& parent = *candidate->frames[pause.parent_depth - 1];
        if (pause.waiting) boundary(parent, pause.wait_next);
        if (pause.selection_ready) {
            if (!pause.waiting || pause.selection_begin >= pause.selection_end) throw std::runtime_error("invalid injected selection state");
            candidate->at(parent, pause.selection_begin); candidate->at(parent, pause.selection_end);
        }
        candidate->injections.push_back(pause);
    }
    if(version>=6)for(uint32_t i=0,n=input.count(uint32_t(max_depth));i<n;++i) {
        Impl::Interrupt pause;pause.depth=uint32_t(input.number());pause.pc=uint32_t(input.number());pause.end=uint32_t(input.number());pause.target=input.optional();
        pause.wait_next=uint32_t(input.number());pause.begin=uint32_t(input.number());pause.selection_end=uint32_t(input.number());
        if(!pause.depth||pause.depth>candidate->frames.size())throw std::runtime_error("invalid selection interrupt depth");
        const auto& f=*candidate->frames[pause.depth-1];
        boundary(f,pause.pc);boundary(f,pause.wait_next);candidate->at(f,pause.end);candidate->at(f,pause.begin);candidate->at(f,pause.selection_end);
        if(pause.begin>=pause.end||pause.end>=pause.selection_end||(pause.target&&*pause.target>=f.script.elements.size()))throw std::runtime_error("invalid selection interrupt state");
        candidate->interrupts.push_back(pause);
    }
    if (input.position != bytes.size()) throw std::runtime_error("trailing VM snapshot data");
    if (candidate->waiting) {
        if (candidate->frames.empty()) throw std::runtime_error("waiting VM snapshot has no context");
        boundary(*candidate->frames.back(), candidate->wait_next);
    }
    if (candidate->selection_ready) {
        if (!candidate->waiting || candidate->selection_begin >= candidate->selection_end)
            throw std::runtime_error("invalid VM snapshot selection state");
        candidate->at(*candidate->frames.back(), candidate->selection_begin);
        candidate->at(*candidate->frames.back(), candidate->selection_end);
    }
    impl_ = std::move(candidate);
}
OriginalSaveState Vm::export_original() const {
    if (impl_->running) throw std::runtime_error("cannot export original VM state during run");
    OriginalSaveState result;
    result.state_header = impl_->native_header;
    result.flags.resize(65536);
    for (const auto& item : impl_->flags) result.flags[size_t(item.first)] = item.second;
    for (const auto& item : impl_->strings) result.strings.emplace(item.first,item.second);
    for (size_t i = 0; i < impl_->frames.size(); ++i) {
        const auto& frame = *impl_->frames[i];
        OriginalFrame saved;
        saved.name = frame.name; saved.current_word = frame.pc; saved.next_word = frame.native_next.value_or(frame.pc);
        saved.target_active = bool(frame.target); saved.target_index = frame.target.value_or(0);
        saved.mode = frame.skip_mode;
        saved.embedded_script = true; saved.script = frame.script; saved.locals = frame.locals;
        saved.header = frame.native_header;
        if(saved.header.empty())saved.header.resize(0x50);
        for(unsigned j=0;j<4;++j){saved.header[0x38+j]=uint8_t(frame.skip_mode>>(8*j));saved.header[0x3c+j]=uint8_t(uint32_t(frame.submenu)>>(8*j));}
        for(unsigned j=0;j<4;++j){saved.header[0x48+j]=uint8_t(uint32_t(frame.select_active)>>(8*j));saved.header[0x4c+j]=uint8_t(uint32_t(frame.select_id)>>(8*j));}
        if (i + 1 == impl_->frames.size() && impl_->waiting) {
            saved.next_word = impl_->wait_next;
            if (impl_->selection_ready) {
                // Native MouseSelect remains at the service instruction while
                // it examines its declarations; Yukari collects to the close.
                const auto opening = frame.words.find(impl_->selection_begin);
                if (opening == frame.words.end() || opening->second == 0)
                    throw std::runtime_error("cannot locate native selection service");
                const auto& service = frame.script.instructions[opening->second - 1];
                if (frame.script.elements[service.element].type != 0)
                    throw std::runtime_error("native selection has no service instruction");
                saved.current_word = service.word_offset; saved.next_word = impl_->selection_begin;
            }
        }
        result.frames.push_back(std::move(saved));
    }
    return result;
}
void Vm::import_original(const OriginalSaveState& state) {
    if (impl_->running) throw std::runtime_error("cannot import original VM state during run");
    if (state.flags.size() != 65536 || state.frames.empty() || state.frames.size() > max_depth)
        throw std::runtime_error("invalid original VM bank/frame counts");
    if (!state.state_header.empty() && state.state_header.size() != 0x160)
        throw std::runtime_error("invalid original VM state header template");
    auto candidate = std::make_unique<Impl>(impl_->loader);
    candidate->host = impl_->host; candidate->trace = impl_->trace;
    candidate->already=impl_->already;candidate->script_state=impl_->script_state;
    // Global banks live in System.dat, not individual native save slots.
    candidate->global_flags = impl_->global_flags; candidate->global_strings = impl_->global_strings;
    candidate->special_strings = impl_->special_strings; candidate->native_header = state.state_header;
    for (uint32_t n = 0; n < 65536; ++n) if (state.flags[n]) candidate->flags.emplace(int32_t(n),state.flags[n]);
    for (const auto& item : state.strings) candidate->set_variable("String",item.first,item.second);
    for (const auto& saved : state.frames) {
        auto frame = candidate->load(saved.name, saved.embedded_script ? &saved.script : nullptr);
        if (saved.locals.size() != frame->locals.size()) throw std::runtime_error("original frame local count mismatch");
        if (!saved.header.empty() && saved.header.size() != 0x50) throw std::runtime_error("invalid original VM frame header template");
        for (const auto& value : saved.locals)
            if (auto text = std::get_if<std::string>(&value); text && text->size() > max_string)
                throw std::runtime_error("original local string exceeds VM limit");
        frame->locals = saved.locals; frame->pc = saved.current_word; frame->native_header = saved.header;
        frame->skip_mode=saved.mode;frame->submenu=!saved.header.empty()&&u32(saved.header,0x3c)!=0;
        if(!saved.header.empty()){frame->select_id=bits(u32(saved.header,0x4c));frame->select_active=u32(saved.header,0x48)!=0;}
        if (saved.next_word != frame->end) candidate->at(*frame,saved.next_word);
        frame->native_next = saved.next_word;
        if (frame->pc != frame->end) {
            if (!saved.embedded_script && !frame->words.count(frame->pc)) {
                // The native reader also scans to an instruction boundary when
                // it must reload an omitted/changed script from game resources.
                uint32_t previous = 0;
                for (const auto& ins : frame->script.instructions) {
                    if (ins.word_offset > frame->pc) break;
                    previous = ins.word_offset;
                }
                frame->pc = previous;
            }
            candidate->at(*frame,frame->pc);
        }
        if (saved.target_active) {
            if (saved.target_index >= frame->script.elements.size()) throw std::runtime_error("original target index out of range");
            const auto type = frame->script.elements[saved.target_index].type;
            if (type != 4 && type != 7 && type != 8 && type != 9 && type != 10)
                throw std::runtime_error("original target is not assignable");
            frame->target = saved.target_index;
        }
        candidate->frames.push_back(std::move(frame));
    }
    impl_ = std::move(candidate);
}
}
