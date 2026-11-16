// Offline, read-only reference listing for one address.
// Usage: -postScript ListReferencesTo.java <output-file> <address>
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;

public class ListReferencesTo extends GhidraScript {
    @Override
    protected void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 2) {
            throw new IllegalArgumentException("output-file address");
        }
        Address target = toAddr(args[1]);
        List<String> lines = new ArrayList<>();
        lines.add("Target: " + currentProgram.getName() + "; SHA256: " +
            currentProgram.getExecutableSHA256());
        lines.add("Address: " + target);
        ReferenceIterator iterator = currentProgram.getReferenceManager()
            .getReferencesTo(target);
        int index = 0;
        while (iterator.hasNext() && !monitor.isCancelled()) {
            Reference reference = iterator.next();
            Function function = getFunctionContaining(reference.getFromAddress());
            lines.add("REF[" + index + "] from=" + reference.getFromAddress() +
                " type=" + reference.getReferenceType() + " source=" +
                reference.getSource() + " function=" +
                (function == null ? "<none>" : function.getEntryPoint() +
                    " " + function.getName()));
            ++index;
        }
        lines.add("References: " + index);
        Files.write(Path.of(args[0]), lines);
        for (String line : lines) {
            println(line);
        }
    }
}
