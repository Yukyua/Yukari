#include "yukari/vm.hpp"
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

namespace {
using namespace yukari;
void check(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
int32_t integer(const Value& value) {
    const auto* result = std::get_if<int32_t>(&value);
    check(result != nullptr, "expected integer variable");
    return *result;
}
void expect(const Vm& vm, const std::string& bank, int32_t index, int32_t value) {
    const auto actual = integer(vm.variable(bank, index));
    check(actual == value, bank + ":" + std::to_string(index) + " expected " +
          std::to_string(value) + ", got " + std::to_string(actual));
}
void execute(Vm& vm, const std::string& script, uint64_t steps) {
    vm.start(script);
    check(vm.run(1000) == VmStatus::Halted, script + " did not halt without a host");
    check(vm.steps() == steps, script + " expected " + std::to_string(steps) +
          " steps, got " + std::to_string(vm.steps()));
    check(std::holds_alternative<std::monostate>(vm.last_result()), script + " unexpectedly returned a value");
}
void auto_skip(const Vm::Loader& loader) {
    const uint64_t expected_steps[] = {15, 17, 19, 12};
    for (int32_t mode = 0; mode < 4; ++mode) {
        Vm vm(loader);
        vm.set_variable("Flag", 10, mode);
        for (int index : {71, 72, 73}) vm.set_variable("GlobalFlag", index, int32_t{-99});
        execute(vm, R"(yks\system\autoskipclear.yks)", expected_steps[mode]);
        expect(vm, "Flag", 10, mode);
        expect(vm, "GlobalFlag", 71, mode == 1 || mode == 2 ? 0 : -99);
        expect(vm, "GlobalFlag", 72, mode == 1 || mode == 2 ? 0 : -99);
        expect(vm, "GlobalFlag", 73, mode == 0 || mode == 2 ? 0 : -99);
    }
}
void state_check(const Vm::Loader& loader) {
    // All later conditions also match: Return must preserve the earliest match.
    // Scenario zero has no match and must fall off the code section.
    for (int32_t selected = 0; selected <= 6; ++selected) {
        Vm vm(loader);
        vm.set_variable("Flag", 520, int32_t{-99});
        vm.set_variable("GlobalFlag", 71, int32_t{-99});
        for (int32_t k = 1; k <= 6; ++k)
            vm.set_variable("Flag", 1490 + k * 10, int32_t{selected && k >= selected ? 4 : 0});
        execute(vm, R"(yks\root\state_chk.yks)", selected ? 9 + 4 * (selected - 1) : 26);
        expect(vm, "Flag", 520, selected);
        expect(vm, "GlobalFlag", 71, -99);
        for (int32_t k = 1; k <= 6; ++k)
            expect(vm, "Flag", 1490 + k * 10, selected && k >= selected ? 4 : 0);
    }
}
void option_defaults(const Vm::Loader& loader) {
    const std::map<int32_t, int32_t> system = {
        {3,0},{4,20},{5,2000},{10,200},{2,0},{6,1},{16,0},{17,1},
        {20,0},{22,0},{23,1},{25,1},{26,1},{40,1},
    };
    std::map<int32_t, int32_t> sound = {{100,500},{101,700},{102,700},{103,700},{28,0}};
    for (int32_t index = 107; index <= 122; ++index) sound[index] = 1000;
    const std::map<int32_t, int32_t> other = {
        {41,1},{30,255},{31,255},{32,255},{33,0},{34,0},{35,0},{85,0},{86,2},{87,0},{88,1},
    };
    const std::map<int32_t, int32_t> groups[] = {system, sound, other};
    const uint64_t expected_steps[] = {108, 39, 57, 41, 17};
    for (int32_t selected = 0; selected <= 4; ++selected) {
        Vm vm(loader);
        vm.set_variable("Flag", 100, selected);
        vm.set_variable("GlobalFlag", 1, int32_t{-99});
        for (const auto& group : groups)
            for (const auto& item : group) vm.set_variable("GlobalFlag", item.first, int32_t{-99});
        execute(vm, R"(yks\system\optiondefault.yks)", expected_steps[selected]);
        expect(vm, "Flag", 100, selected);
        expect(vm, "GlobalFlag", 1, -99);
        for (int32_t group = 0; group < 3; ++group)
            for (const auto& item : groups[group])
                expect(vm, "GlobalFlag", item.first,
                       selected == 0 || selected == group + 1 ? item.second : -99);
    }
}
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: yukari_vm_game_tests ORIGINAL_GAME_DIRECTORY\n";
        return 2;
    }
    try {
        const Resources resources(argv[1]);
        const Vm::Loader loader = [&](const std::string& name) { return parse_script(resources.read(name)); };
        auto_skip(loader);
        state_check(loader);
        option_defaults(loader);
        std::cout << "PASS 16 original-script scenarios without host stubs\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
    return 0;
}
