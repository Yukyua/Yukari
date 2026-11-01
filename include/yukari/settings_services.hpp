#pragma once
#include "yukari/vm.hpp"
#include <map>
namespace yukari {
// These two fields belong to the current script frame in the original engine.
// The runtime must supply the corresponding frame for every call, preserving
// ScriptCall context isolation rather than treating these fields as globals.
struct SettingsFrame { int32_t script_skip_mode{};bool submenu_enabled{}; };
struct SettingsState {
    std::map<std::string,Value> values;
    std::map<std::string,std::vector<Value>> records;
};
struct SettingsHooks {
    // Called after a concrete state change; consumers can query all typed values
    // and per-command records to update actual text/selection/history/layout UI.
    std::function<void(const std::string&,const SettingsState&)> changed;
    std::function<bool()> fullscreen_get;
    std::function<void(bool)> fullscreen_set;
};
class SettingsServices {
public:
    // Initial values follow original globals/context constructor. The one
    // omitted native byte (AlreadyNoWaitMode) is explicitly disabled for a
    // deterministic port; see instruction-contracts.md. Unknown property
    // queries throw, while all implemented native Get commands are initialized.
    explicit SettingsServices(SettingsHooks hooks={});
    std::optional<Value> call(const std::string& name,const std::vector<Value>& args,SettingsFrame* frame=nullptr);
    const Value& query(const std::string& key) const;
    int32_t integer(const std::string& key) const;
    const std::vector<Value>& record(const std::string& key) const;
    bool key_matches(const std::string& binding,int32_t virtual_key,bool repeated=false) const;
    void seed(const std::string& key,Value value);
    const SettingsState& state() const {return state_;}
    void restore(SettingsState state);
private:
    SettingsHooks hooks_;
    SettingsState state_;
    void changed(const std::string& name);
};
}
