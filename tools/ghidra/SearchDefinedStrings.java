// Offline, read-only search of Ghidra-defined string data and its references.
// Usage: -postScript SearchDefinedStrings.java <output-file> <needle>
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Data;
import ghidra.program.model.listing.DataIterator;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;

public class SearchDefinedStrings extends GhidraScript {
    @Override
    protected void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 2) throw new IllegalArgumentException("output-file needle");
        String needle = args[1].toLowerCase();
        List<String> lines = new ArrayList<>();
        lines.add("Target: " + currentProgram.getName() + "; SHA256: " +
            currentProgram.getExecutableSHA256());
        DataIterator iterator = currentProgram.getListing().getDefinedData(true);
        int matches = 0;
        while (iterator.hasNext() && !monitor.isCancelled()) {
            Data data = iterator.next();
            Object value = data.getValue();
            String rendered = value == null ? data.getDefaultValueRepresentation() : value.toString();
            if (rendered == null || !rendered.toLowerCase().contains(needle)) continue;
            lines.add("MATCH[" + matches + "] address=" + data.getAddress() + " value=" + rendered);
            ReferenceIterator refs = currentProgram.getReferenceManager().getReferencesTo(data.getAddress());
            while (refs.hasNext()) {
                Reference ref = refs.next();
                Function function = getFunctionContaining(ref.getFromAddress());
                lines.add("  REF from=" + ref.getFromAddress() + " type=" +
                    ref.getReferenceType() + " function=" +
                    (function == null ? "<none>" : function.getEntryPoint() + " " + function.getName()));
            }
            ++matches;
        }
        lines.add("Matches: " + matches);
        Files.write(Path.of(args[0]), lines);
        for (String line : lines) println(line);
    }
}
