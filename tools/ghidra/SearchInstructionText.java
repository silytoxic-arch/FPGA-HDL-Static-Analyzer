// Offline, read-only search over Ghidra's decoded instruction listing.
// Usage: -postScript SearchInstructionText.java <output-file> <needle>
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

public class SearchInstructionText extends GhidraScript {
    @Override
    protected void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 2) {
            throw new IllegalArgumentException("output-file needle");
        }
        String needle = args[1].toLowerCase(Locale.ROOT);
        List<String> lines = new ArrayList<>();
        lines.add("Target: " + currentProgram.getName() + "; SHA256: " +
            currentProgram.getExecutableSHA256());
        lines.add("Needle: " + needle);
        InstructionIterator iterator = currentProgram.getListing()
            .getInstructions(true);
        int matches = 0;
        while (iterator.hasNext() && !monitor.isCancelled()) {
            Instruction instruction = iterator.next();
            if (!instruction.toString().toLowerCase(Locale.ROOT).contains(needle)) {
                continue;
            }
            Function function = getFunctionContaining(instruction.getAddress());
            lines.add("MATCH[" + matches + "] " + instruction.getAddress() +
                " function=" + (function == null ? "<none>" :
                    function.getEntryPoint() + " " + function.getName()));
            Instruction cursor = instruction;
            for (int before = 0; before < 3 && cursor.getPrevious() != null; ++before) {
                cursor = cursor.getPrevious();
            }
            for (int context = 0; context < 7 && cursor != null; ++context) {
                lines.add("  " + cursor.getAddress() + "  " + cursor);
                cursor = cursor.getNext();
            }
            ++matches;
        }
        lines.add("Matches: " + matches);
        Files.write(Path.of(args[0]), lines);
        for (String line : lines) {
            println(line);
        }
    }
}
