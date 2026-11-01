#pragma once
#include "yukari/formats.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <variant>

namespace yukari {
using Value = std::variant<std::monostate, int32_t, std::string>;
struct VmLocation { std::string script; uint32_t word; };
struct HostCall {
    std::string function;
    VmLocation location;
    std::vector<Value> arguments;
    std::optional<uint32_t> block_begin, block_end;
    bool subcall = false; // Native current-frame +0xc9 flag.
};
struct HostResult {
    // Waiting suspends the call exactly once. resume(value) completes it.
    bool waiting = false;
    Value value{};
    // Evaluate declarations in the following block, observing Case calls,
    // then wait at its closing label for an input branch.
    bool collect_block = false;
    // If the service has a following {...} block, bypass its body. A waiting
    // call remembers the same destination until resume() completes it.
    bool skip_block = false;
};
enum class VmStatus { Halted, Waiting, StepLimit };
struct OriginalSaveState;
class Vm {
public:
    using Loader = std::function<Script(const std::string&)>;
    using Host = std::function<std::optional<HostResult>(const HostCall&)>;
    using Trace = std::function<void(const VmLocation&, const std::string&)>;
    explicit Vm(Loader loader);
    ~Vm();
    Vm(const Vm&) = delete;
    Vm& operator=(const Vm&) = delete;
    void set_host(Host handler);
    void set_trace(Trace trace);
    // Starts a new execution; shared variable banks survive between starts.
    void start(const std::string& entry);
    // Discard execution and ordinary banks; keep globals, read history and callbacks.
    void reset(const std::string& entry);
    // Inject a script above the current paused context. Its completion restores
    // the caller's previous wait/selection state; variable banks remain shared.
    void call(const std::string& entry,bool subcall=false);
    bool in_call() const;
    bool in_subcall() const;
    bool pending_subcall() const;
    // Native process-global DAT_0056ca70, changed by event-script dispatch.
    int32_t script_state() const;
    void set_script_state(int32_t state);
    uint32_t current_frame_skip_mode() const;
    void set_current_frame_skip_mode(uint32_t mode);
    bool submenu_enabled() const;
    void set_submenu_enabled(bool enabled);
    std::optional<int32_t> current_frame_select_id() const;
    void set_current_frame_select_id(int32_t id);
    VmStatus run(uint64_t step_limit = 100000);
    void resume(Value result = {});
    void choose(uint32_t word);
    // Execute an event block in the selecting frame, then restore its wait.
    void interrupt_selection(uint32_t branch_word, uint32_t block_end);
    Value variable(const std::string& bank, int32_t index) const;
    void set_variable(const std::string& bank, int32_t index, Value value);
    // FlagReset: ordinary Flag/String banks only. Frame locals, global banks,
    // read history and the running instruction remain intact.
    void clear_variables();
    // Global read history is separate from save-slot snapshots. Loading a slot
    // never rolls it back. current_already is the old byte for the latest
    // instruction, read immediately before that instruction is marked visited.
    bool current_already() const;
    bool is_already(const std::string& script,uint32_t word) const;
    void load_already(const Bytes& bytes,const char* encoding = "CP932");
    Bytes save_already(const char* encoding = "CP932") const;
    // Diagnostic result of the root Return. Child Return values are not
    // forwarded to ScriptCall targets in the original interpreter.
    const Value& last_result() const;
    uint64_t steps() const;
    // Stable, versioned VM state; call outside run(). Restore is transactional
    // and requires the same source scripts through the configured loader.
    Bytes snapshot() const;
    // Checkpoint at the continuation of a suspended ScriptSubCall, excluding
    // the menu child. Does not alter the live VM or execute the child.
    Bytes snapshot_after_subcall() const;
    void restore(const Bytes& bytes);
    OriginalSaveState export_original() const;
    void import_original(const OriginalSaveState& state);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
std::string format_value(const Value& value);
}
