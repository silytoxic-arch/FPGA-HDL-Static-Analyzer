// Read-only targeted Ghidra helper for a known function entry in a temporary
// imported program. It disassembles reachable code, creates a function when
// auto-analysis was deliberately skipped, and writes decompiler output.
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;

public class TargetedDecompile extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 2)
            throw new IllegalArgumentException("<address>... <output-file>");
        List<String> lines = new ArrayList<>();
        lines.add("Target: " + currentProgram.getName());
        DecompInterface decompiler = new DecompInterface();
        decompiler.openProgram(currentProgram);
        try {
            for (int index = 0; index + 1 < args.length; ++index) {
                String raw = args[index].replaceFirst("^0[xX]", "");
                Address address = currentProgram.getAddressFactory()
                    .getDefaultAddressSpace().getAddress(Long.parseUnsignedLong(raw, 16));
                Function function = getFunctionAt(address);
                if (function == null) {
                    disassemble(address);
                    function = createFunction(address, null);
                }
                lines.add("");
                lines.add("Address: " + args[index]);
                lines.add("Function: " +
                          (function == null ? "<missing>" : function.getName()));
                if (function == null) continue;
                DecompileResults result = decompiler.decompileFunction(function, 120, monitor);
                lines.add("completed=" + result.decompileCompleted() + "; error=" +
                          result.getErrorMessage());
                if (result.getDecompiledFunction() != null)
                    lines.add(result.getDecompiledFunction().getC());
            }
        } finally {
            decompiler.dispose();
        }
        Files.write(Path.of(args[args.length - 1]), lines);
        for (String line : lines) println(line);
    }
}
