#include "analysis/violation.h"
#include "core/project.h"
#include "database/design_database.h"

#include <iostream>
#include <string>

namespace {
class TestContext {
public:
    void check(bool condition, const char* message) {
        if (!condition) {
            std::cerr << "FAIL: " << message << '\n';
            ++failures_;
        }
    }

    [[nodiscard]] int result() const noexcept { return failures_ == 0 ? 0 : 1; }

private:
    int failures_{0};
};

fpga::core::SourceLocation location(std::size_t line, std::size_t column) {
    return {"rtl/top.sv", line, column, line * 10 + column};
}
} // namespace

int main() {
    using namespace fpga;
    TestContext test;

    core::Project project("demo", "rtl");
    test.check(project.addSourceFile(
        core::SourceFile("rtl/./top.sv", core::HdlLanguage::SystemVerilog,
                         "module top; endmodule")),
               "first source file is accepted");
    test.check(!project.addSourceFile(
        core::SourceFile("rtl/top.sv", core::HdlLanguage::SystemVerilog)),
               "lexically equivalent source path is rejected");
    const auto source = project.findSourceFile("rtl/top.sv");
    test.check(source.has_value(), "source file can be found");
    test.check(source && source->get().content().find("module top") != std::string::npos,
               "source file owns its content");
    test.check(location(4, 8).valid(), "one-based source location is valid");

    design::Signal clockSignal("clk", 1, false, design::SignalType::Logic,
                               "top", location(2, 9));
    clockSignal.addDriver({"port", "clk", location(2, 9)});
    clockSignal.addLoad({"always", "state register", location(12, 5)});
    clockSignal.setClockDomain("system");

    design::Signal stateSignal("state", 4, false, design::SignalType::Logic,
                               "top", location(5, 15));
    stateSignal.addDriver({"nonblocking assignment", "next_state", location(13, 9)});
    stateSignal.addLoad({"expression", "output decode", location(20, 12)});
    stateSignal.setClockDomain("system");
    stateSignal.setResetDomain("power_on");

    design::Module top("top", "rtl/top.sv", location(1, 1));
    top.addPort({"clk", design::PortDirection::Input, 1, false, "clk", location(2, 9)});
    top.addSignal(std::move(clockSignal));
    top.addSignal(std::move(stateSignal));
    top.addParameter({"WIDTH", "4", std::string("int"), location(1, 20)});
    top.addInstance({"u_child", "child", "top", location(25, 3)});
    top.addAssignment({"ready", "state == 4'd0", true, location(20, 3)});
    top.addAlwaysBlock({design::AlwaysKind::Sequential, {"posedge clk"},
                        {{"state", "next_state", false, location(13, 9)}},
                        location(11, 3)});
    top.addClock({"system", "clk", 10.0, location(2, 9)});
    top.addReset({"power_on", "rst_n", false, true, location(3, 9)});

    design::DesignDatabase database;
    test.check(database.addModule(std::move(top)), "module is added");
    test.check(!database.addModule(design::Module("top", "rtl/duplicate.sv")),
               "duplicate module is rejected");
    test.check(!database.addModule(design::Module("", "rtl/unnamed.sv")),
               "empty module name is rejected");

    const auto foundTop = database.findModule("top");
    test.check(foundTop.has_value(), "module can be found");
    test.check(!database.findModule("missing"), "missing module is reported");
    test.check(foundTop && foundTop->get().ports().size() == 1, "module owns ports");
    test.check(foundTop && foundTop->get().parameters().size() == 1,
               "module owns parameters");
    test.check(foundTop && foundTop->get().assignments().size() == 1,
               "module owns assignments");
    test.check(foundTop && foundTop->get().alwaysBlocks().size() == 1,
               "module owns always blocks");
    test.check(foundTop && foundTop->get().clocks().size() == 1, "module owns clocks");
    test.check(foundTop && foundTop->get().resets().size() == 1, "module owns resets");

    const auto state = database.findSignal("top", "state");
    test.check(state.has_value(), "signal can be found within its module");
    test.check(!database.findSignal("top", "missing"), "missing signal is reported");
    test.check(!database.findSignal("missing", "state"), "missing parent is reported");
    test.check(state && state->get().width() == 4, "signal width is retained");
    test.check(state && !state->get().isSigned(), "signal signedness is retained");
    test.check(state && state->get().type() == design::SignalType::Logic,
               "signal type is retained");
    test.check(state && state->get().module() == "top", "signal module is retained");
    test.check(state && state->get().clockDomain() == "system", "clock domain is retained");
    test.check(state && state->get().resetDomain() == "power_on", "reset domain is retained");

    test.check(database.getModules().size() == 1, "all modules query works");
    test.check(database.getSignals().size() == 2, "all signals query works");
    test.check(database.getInstances().size() == 1, "all instances query works");
    test.check(database.getDrivers().size() == 2, "all drivers query works");
    test.check(database.getLoads().size() == 2, "all loads query works");

    const auto* stableAddress = &foundTop->get();
    test.check(database.addModule(design::Module("child", "rtl/child.sv")),
               "second module is added");
    test.check(&database.findModule("top")->get() == stableAddress,
               "module reference remains stable after insertion");
    test.check(database.getModules()[0].get().name() == "child" &&
                   database.getModules()[1].get().name() == "top",
               "module query order is deterministic");

    analysis::Violation violation{
        "HDL-001", analysis::Severity::Warning, "Implicit net", "Net is undeclared",
        "Declare the net explicitly", "rtl/top.sv", 20, 3,
        std::string("top"), std::string("ready")};
    test.check(violation.severity == analysis::Severity::Warning,
               "violation severity is retained");
    test.check(violation.module == "top" && violation.signal == "ready",
               "violation context is retained");

    return test.result();
}
