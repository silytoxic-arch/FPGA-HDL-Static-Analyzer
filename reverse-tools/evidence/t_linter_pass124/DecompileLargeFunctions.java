// Offline, read-only decompilation of explicit function addresses.
// Usage: -postScript DecompileFunctions.java <output-file> <address>...
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileOptions;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;

public class DecompileLargeFunctions extends GhidraScript {
    @Override
    protected void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 2) {
            throw new IllegalArgumentException("output-file address...");
        }
        List<String> lines = new ArrayList<>();
        lines.add("Target: " + currentProgram.getName() + "; SHA256: " +
            currentProgram.getExecutableSHA256());
        DecompInterface decompiler = new DecompInterface();
        DecompileOptions options = new DecompileOptions();
        options.setMaxPayloadMBytes(128);
        options.setMaxInstructions(1000000);
        decompiler.setOptions(options);
        lines.add("Decompiler payload limit: 128 MiB; instruction limit: 1000000");
        decompiler.openProgram(currentProgram);
        try {
            for (int i = 1; i < args.length; ++i) {
                Function function = getFunctionAt(toAddr(args[i]));
                if (function == null) {
                    function = getFunctionContaining(toAddr(args[i]));
                }
                lines.add("\n===== " + args[i] + " " +
                    (function == null ? "<no-function>" : function.getEntryPoint() + " " + function.getName()) +
                    " =====");
                if (function == null) {
                    continue;
                }
                DecompileResults result = decompiler.decompileFunction(function, 120, monitor);
                if (!result.decompileCompleted()) {
                    lines.add("DECOMPILE_ERROR: " + result.getErrorMessage());
                    continue;
                }
                lines.add(result.getDecompiledFunction().getC());
            }
        } finally {
            decompiler.dispose();
        }
        Files.write(Path.of(args[0]), lines);
        for (String line : lines) {
            println(line);
        }
    }
}
