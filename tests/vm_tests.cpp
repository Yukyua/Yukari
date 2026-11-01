#include "yukari/vm.hpp"
#include <climits>
#include <functional>
#include <iostream>
#include <map>
#include <stdexcept>
#include <utility>

namespace {
using namespace yukari;

void check(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
int32_t integer(const Value& value) {
    const auto* number = std::get_if<int32_t>(&value);
    check(number != nullptr, "expected an integer Value");
    return *number;
}

// Element indices and word addresses are separate. Calls occupy 2+argc words;
// targets and labels occupy one. Label links are patched after layout is known.
struct Program {
    Script script;
    uint32_t next_word = 0;
    uint32_t add(uint32_t type, std::string text = {}, int32_t value = 0, uint32_t b = 0) {
        const auto result = static_cast<uint32_t>(script.elements.size());
        script.elements.push_back({type, 0, b, 0, std::move(text), value});
        return result;
    }
    uint32_t number(int32_t value) {
        // Real YKS stores each integer at a data-section offset. Distinct values
        // must not accidentally claim the same offset in an in-memory fixture.
        const auto offset = static_cast<uint32_t>(script.elements.size()) * 4;
        return add(4, {}, value, offset);
    }
    uint32_t string(const std::string& value) { return add(5, value); }
    uint32_t label(const std::string& name, int32_t link = -1) { return add(1, name, link); }
    uint32_t local(uint32_t number) {
        if (script.max_locals <= number) script.max_locals = number + 1;
        return add(10, {}, 0, number);
    }
    uint32_t bank(const std::string& name, int32_t number) { return add(8, name, number); }
    uint32_t emit(uint32_t element, std::vector<uint32_t> arguments = {}) {
        const auto address = next_word;
        next_word += script.elements.at(element).type == 0 ? 2 + static_cast<uint32_t>(arguments.size()) : 1;
        script.instructions.push_back({address, element, std::move(arguments)});
        return address;
    }
    uint32_t call(const std::string& name, std::vector<uint32_t> args = {}) {
        return emit(add(0, name), std::move(args));
    }
    void assign(uint32_t destination, std::vector<uint32_t> expression) {
        emit(destination);
        call("=", std::move(expression));
    }
};

Vm::Loader loader(std::map<std::string, Script> programs) {
    return [programs = std::move(programs)](const std::string& name) { return programs.at(name); };
}

using Token = std::variant<int32_t, const char*>;
int32_t calculate(std::initializer_list<Token> expression) {
    Program p;
    auto destination = p.local(0);
    std::vector<uint32_t> args;
    for (const auto& token : expression) {
        if (auto number = std::get_if<int32_t>(&token)) args.push_back(p.number(*number));
        else args.push_back(p.label(std::get<const char*>(token), 65535));
    }
    p.assign(destination, std::move(args));
    p.call("Return", {destination});
    Vm vm(loader({{"arithmetic.yks", p.script}}));
    vm.start("arithmetic.yks");
    check(vm.run() == VmStatus::Halted, "arithmetic did not halt");
    return integer(vm.last_result());
}

void arithmetic() {
    check(calculate({2, "+", 3, "*", 4}) == 14, "multiplication must precede addition");
    check(calculate({20, "-", 6, "/", 2}) == 17, "division must precede subtraction");
    check(calculate({9, "-", 4, "-", 2}) == 3, "subtraction must associate left");
    check(calculate({24, "/", 4, "*", 2}) == 12, "division/multiplication must associate left");
    check(calculate({2, "+", 3, "=", 5}) == 1, "arithmetic must precede equality");
    check(calculate({2, "+", 3, "<", 6}) == 1, "arithmetic must precede less-than");
    check(calculate({2, "+", 3, ">", 6}) == 0, "arithmetic must precede greater-than");
    check(calculate({1, "<", 2, "=", 1}) == 1, "comparisons must associate left");
    check(calculate({INT32_MAX, "+", 1}) == INT32_MIN, "addition must wrap at 32 bits");
    check(calculate({INT32_MIN, "-", 1}) == INT32_MAX, "subtraction must wrap at 32 bits");
    check(calculate({1073741824, "*", 4}) == 0, "multiplication must wrap at 32 bits");
}

void conditionals() {
    for (int32_t condition : {0, 1}) {
        Program p;
        const auto flag = p.bank("Flag", 0);
        const auto yes_open = p.label("{"), yes_close = p.label("}");
        const auto else_mark = p.label("else"), no_open = p.label("{"), no_close = p.label("}");
        p.call("if", {p.number(condition)});
        const auto yes_address = p.emit(yes_open);
        p.assign(flag, {p.number(10)});
        p.emit(yes_close);
        p.emit(else_mark);
        const auto no_address = p.emit(no_open);
        p.assign(flag, {p.number(20)});
        const auto end_address = p.emit(no_close);
        p.call("Return", {flag});
        p.script.elements[yes_open].value = static_cast<int32_t>(no_address);
        p.script.elements[yes_close].value = static_cast<int32_t>(yes_address);
        p.script.elements[no_open].value = static_cast<int32_t>(end_address);
        p.script.elements[no_close].value = static_cast<int32_t>(no_address);
        Vm vm(loader({{"conditional.yks", p.script}}));
        vm.start("conditional.yks");
        check(vm.run() == VmStatus::Halted, "conditional did not halt");
        check(integer(vm.last_result()) == (condition ? 10 : 20), "if/else selected wrong branch");
    }
}

void call_contexts() {
    Program p;
    const auto local = p.local(0), shared = p.bank("Flag", 0), child = p.label("CHILD");
    p.assign(local, {p.number(7)});
    p.assign(shared, {p.number(1)});
    p.call("ScriptCall", {child});
    p.call("Return", {local});
    const auto child_address = p.emit(child);
    p.script.elements[child].value = static_cast<int32_t>(child_address);
    p.assign(local, {p.number(99)});
    p.assign(shared, {p.number(2)});
    p.call("Return");
    Vm vm(loader({{"contexts.yks", p.script}}));
    vm.start("contexts.yks");
    check(vm.run() == VmStatus::Halted, "ScriptCall did not return");
    check(integer(vm.last_result()) == 7, "same-file ScriptCall must have fresh local storage");
    check(integer(vm.variable("Flag", 0)) == 2, "Flag storage must be shared across ScriptCall");

    Program source, destination;
    source.call("ScriptJump", {source.string("destination.yks")});
    source.assign(source.bank("Flag", 5), {source.number(99)});
    source.call("Return");
    destination.assign(destination.bank("Flag", 5), {destination.number(3)});
    destination.call("Return");
    Vm jump(loader({{"source.yks", source.script}, {"destination.yks", destination.script}}));
    jump.start("source.yks");
    check(jump.run() == VmStatus::Halted, "cross-file ScriptJump did not halt");
    check(integer(jump.variable("Flag", 5)) == 3, "cross-file ScriptJump resumed its source");
}

void host_results() {
    for (bool asynchronous : {false, true}) {
        Program p;
        const auto local = p.local(0);
        p.emit(local);
        p.call("=");
        p.call("ReadValue", {p.number(12)});
        p.call("Return", {local});
        Vm vm(loader({{"host.yks", p.script}}));
        int calls = 0;
        vm.set_host([&](const HostCall& call) -> std::optional<HostResult> {
            ++calls;
            check(call.function == "ReadValue", "core VM call leaked to host");
            check(call.location.script == "host.yks" && call.location.word == 3, "incorrect host location");
            check(call.arguments.size() == 1 && integer(call.arguments[0]) == 12, "incorrect host arguments");
            return HostResult{asynchronous, asynchronous ? Value{} : Value{int32_t{42}}};
        });
        vm.start("host.yks");
        if (asynchronous) {
            check(vm.run() == VmStatus::Waiting, "waiting host result did not suspend");
            check(vm.run() == VmStatus::Waiting, "run must remain suspended until resume");
            check(calls == 1, "waiting host call was executed more than once");
            vm.resume(int32_t{42});
        }
        check(vm.run() == VmStatus::Halted, "host result did not complete");
        check(calls == 1, "host must execute exactly once");
        check(integer(vm.last_result()) == 42, "=() lost the target for host/resume result");
    }
}

std::string failure(const std::function<void()>& work) {
    try { work(); }
    catch (const std::runtime_error& error) { return error.what(); }
    throw std::runtime_error("expected runtime_error was not thrown");
}

void errors_and_limits() {
    Program unknown;
    unknown.call("MissingEngineFunction");
    Vm vm(loader({{"unknown.yks", unknown.script}}));
    vm.start("unknown.yks");
    const auto message = failure([&] { vm.run(); });
    check(message.find("unknown.yks") != std::string::npos, "unknown function error omitted script");
    check(message.find("MissingEngineFunction") != std::string::npos, "unknown function error omitted name");
    check(message.find('0') != std::string::npos, "unknown function error omitted word offset");

    Program loop;
    const auto forever = loop.label("FOREVER", 0);
    loop.emit(forever);
    loop.call("ScriptJump", {forever});
    Vm limited(loader({{"loop.yks", loop.script}}));
    limited.start("loop.yks");
    check(limited.run(20) == VmStatus::StepLimit, "unbounded loop ignored step limit");
    check(limited.steps() > 0 && limited.steps() <= 20, "step counter exceeded budget");

    Program bad;
    const auto inside_argument = bad.label("BROKEN", 1);
    bad.call("ScriptJump", {inside_argument});
    bad.call("Return");
    const auto bad_message = failure([&] {
        Vm invalid(loader({{"badlink.yks", bad.script}}));
        invalid.start("badlink.yks");
        invalid.run();
    });
    check(!bad_message.empty(), "jump into argument word must be rejected");

    Program uninitialized;
    uninitialized.call("Return", {uninitialized.local(0)});
    const auto local_message = failure([&] {
        Vm invalid(loader({{"uninitialized.yks", uninitialized.script}}));
        invalid.start("uninitialized.yks");
        invalid.run();
    });
    check(local_message.find("uninitialized") != std::string::npos,
          "reading an uninitialized local must fail explicitly");
}

void string_expressions() {
    Program p;
    const auto text = p.add(9, "String", 0), empty = p.add(9, "String", 1);
    p.assign(text, {p.string("a"), p.label("+", 65535), p.string("b"),
                    p.label("+", 65535), p.string("c")});
    // Unlike assigning one empty literal, a multi-argument empty concatenation
    // returns no value in the original and must leave the variable unchanged.
    p.assign(text, {p.string(""), p.label("+", 65535), p.string("")});
    p.assign(empty, {p.string("keep")});
    p.assign(empty, {p.string("")});
    const auto equal = p.local(0), unequal = p.local(1);
    p.assign(equal, {text, p.label("=", 65535), p.string("abc")});
    p.assign(unequal, {text, p.label("=", 65535), p.string("different")});
    p.assign(p.bank("Flag", 0), {unequal});
    p.call("Return", {equal});
    Vm vm(loader({{"strings.yks", p.script}}));
    vm.start("strings.yks");
    check(vm.run() == VmStatus::Halted, "string program did not halt");
    check(std::get<std::string>(vm.variable("String", 0)) == "abc", "concat or empty-concat assignment is incorrect");
    check(std::get<std::string>(vm.variable("String", 1)).empty(), "single empty literal must clear a string");
    check(integer(vm.last_result()) == 1, "equal strings must compare true");
    check(integer(vm.variable("Flag", 0)) == 0, "different strings must compare false");
}

void shared_data_alias() {
    Program p;
    const auto alias = p.add(4, {}, 3, 0);
    const auto nine = p.add(4, {}, 9, 4);
    const auto answer = p.add(4, {}, 42, 8);
    const auto variable = p.bank("Flag", 3);
    // Flag's ID DWORD and the integer assignment target share data offset zero.
    // The decoded Element::value is deliberately stale after that assignment.
    p.script.elements[variable].c = 0;
    p.script.data = {3,0,0,0, 9,0,0,0, 42,0,0,0};
    p.assign(alias, {nine});
    p.assign(variable, {answer});
    p.call("Return");
    Vm vm(loader({{"data-alias.yks", p.script}}));
    vm.set_variable("Flag", 3, int32_t{-99});
    vm.set_variable("Flag", 9, int32_t{-99});
    vm.start("data-alias.yks");
    check(vm.run() == VmStatus::Halted, "data-alias program did not halt");
    check(integer(vm.variable("Flag", 3)) == -99, "assignment used a stale cached variable ID");
    check(integer(vm.variable("Flag", 9)) == 42, "integer data write did not update aliased variable ID");
}

void ordinary_bank_reset() {
    Program p;const auto local=p.local(0);p.assign(local,{p.number(73)});
    p.call("ResetBanks");p.call("Return",{local});
    Vm vm(loader({{"reset.yks",p.script}}));
    vm.set_variable("Flag",1,int32_t(8));vm.set_variable("String",2,std::string("old"));
    vm.set_variable("GlobalFlag",1,int32_t(19));vm.set_variable("GlobalString",2,std::string("keep"));
    vm.set_host([&](const HostCall&)->std::optional<HostResult>{vm.clear_variables();return HostResult{};});
    vm.start("reset.yks");check(vm.run()==VmStatus::Halted,"bank reset interrupted execution");
    check(integer(vm.variable("Flag",1))==0&&std::get<std::string>(vm.variable("String",2)).empty(),"ordinary banks not cleared");
    check(integer(vm.variable("GlobalFlag",1))==19&&std::get<std::string>(vm.variable("GlobalString",2))=="keep","global banks cleared");
    check(integer(vm.last_result())==73&&vm.is_already("reset.yks",0),"bank reset cleared locals/read history");
    check(vm.snapshot().size()<4096,"empty bank entries expanded snapshot");
}
void selection_collection() {
    Program p;
    const auto outer = p.label("{"), outer_close = p.label("}");
    const auto first = p.label("{"), first_close = p.label("}");
    const auto second = p.label("{"), second_close = p.label("}");
    const auto x = p.local(0), y = p.local(1);
    const auto flag0 = p.bank("Flag", 0), flag1 = p.bank("Flag", 1);
    p.call("MouseSelect");
    const auto begin = p.emit(outer);
    p.assign(x, {p.number(2), p.label("+", 65535), p.number(3)});
    const auto case_word = p.call("MouseCase2", {x});
    const auto first_begin = p.emit(first);
    p.assign(flag0, {p.number(11)});
    p.call("Return", {p.number(11)});
    const auto first_end = p.emit(first_close);
    p.assign(y, {x, p.label("*", 65535), p.number(2)});
    p.call("MouseCase2", {y});
    const auto second_begin = p.emit(second);
    p.assign(flag1, {p.number(22)});
    p.call("Return", {p.number(22)});
    const auto second_end = p.emit(second_close);
    const auto end = p.emit(outer_close);
    p.call("Return");
    p.script.elements[outer].value = static_cast<int32_t>(end);
    p.script.elements[outer_close].value = static_cast<int32_t>(begin);
    p.script.elements[first].value = static_cast<int32_t>(first_end);
    p.script.elements[first_close].value = static_cast<int32_t>(first_begin);
    p.script.elements[second].value = static_cast<int32_t>(second_end);
    p.script.elements[second_close].value = static_cast<int32_t>(second_begin);

    for (unsigned selected : {0U, 1U}) {
        Vm vm(loader({{"selection.yks", p.script}}));
        unsigned select_calls = 0;
        std::vector<HostCall> cases;
        vm.set_host([&](const HostCall& call) -> std::optional<HostResult> {
            if (call.function == "MouseSelect") {
                ++select_calls;
                check(call.block_begin == begin && call.block_end == end, "selection block locations are incorrect");
                return HostResult{false, {}, true};
            }
            check(call.function == "MouseCase2", "unexpected selection host callback");
            cases.push_back(call);
            return HostResult{};
        });
        vm.start("selection.yks");
        check(vm.run(100) == VmStatus::Waiting, "selection declarations did not reach input wait");
        check(select_calls == 1 && cases.size() == 2, "selection declarations were not observed exactly once");
        check(cases[0].arguments.size() == 1 && integer(cases[0].arguments[0]) == 5,
              "first Case did not receive its computed declaration value");
        check(cases[1].arguments.size() == 1 && integer(cases[1].arguments[0]) == 10,
              "second Case did not receive its computed declaration value");
        check(cases[0].block_begin == first_begin && cases[0].block_end == first_end,
              "first Case block locations are incorrect");
        check(cases[1].block_begin == second_begin && cases[1].block_end == second_end,
              "second Case block locations are incorrect");
        check(integer(vm.variable("Flag", 0)) == 0 && integer(vm.variable("Flag", 1)) == 0,
              "a selection body executed before input");
        const auto steps = vm.steps();
        check(vm.run(100) == VmStatus::Waiting && vm.steps() == steps,
              "a pending selection must not execute or spin on repeated run");
        check(select_calls == 1 && cases.size() == 2, "pending selection repeated host callbacks");
        failure([&] { vm.choose(begin); });
        failure([&] { vm.choose(end + 1); });
        failure([&] { vm.choose(case_word + 1); });
        check(vm.run() == VmStatus::Waiting, "invalid choose consumed the pending selection");
        vm.choose(selected ? second_begin : first_begin);
        check(vm.run() == VmStatus::Halted, "chosen branch did not execute and halt");
        check(integer(vm.last_result()) == (selected ? 22 : 11), "wrong selected branch returned");
        check(integer(vm.variable("Flag", 0)) == (selected ? 0 : 11), "first body executed incorrectly");
        check(integer(vm.variable("Flag", 1)) == (selected ? 22 : 0), "second body executed incorrectly");
        check(select_calls == 1 && cases.size() == 2, "choosing a branch replayed declarations");
    }
}
}

int main() {
    const std::pair<const char*, std::function<void()>> tests[] = {
        {"arithmetic", arithmetic}, {"conditionals", conditionals},
        {"call contexts", call_contexts}, {"host results", host_results},
        {"errors and limits", errors_and_limits}, {"string expressions", string_expressions},
        {"shared data alias", shared_data_alias}, {"ordinary bank reset",ordinary_bank_reset},
        {"selection collection", selection_collection},
    };
    unsigned failed = 0;
    for (const auto& test : tests) {
        try { test.second(); std::cout << "PASS " << test.first << '\n'; }
        catch (const std::exception& error) {
            ++failed;
            std::cerr << "FAIL " << test.first << ": " << error.what() << '\n';
        }
    }
    return failed ? 1 : 0;
}
