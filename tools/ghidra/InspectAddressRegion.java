// Offline, read-only inspection of bytes, pointers, symbols, data and references.
// Usage: -postScript InspectAddressRegion.java <output-file> <start-address> <length>
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Data;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator;
import ghidra.program.model.symbol.Symbol;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;

public class InspectAddressRegion extends GhidraScript {
    private String symbolAt(Address address) {
        Symbol symbol = currentProgram.getSymbolTable().getPrimarySymbol(address);
        return symbol == null ? "" : " symbol=" + symbol.getName(true);
    }

    @Override
    protected void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 3) {
            throw new IllegalArgumentException("output-file start-address length");
        }
        Address start = toAddr(args[1]);
        int length = Integer.decode(args[2]);
        List<String> lines = new ArrayList<>();
        lines.add("Target: " + currentProgram.getName() + "; SHA256: " +
            currentProgram.getExecutableSHA256());
        lines.add("Region: " + start + " length=0x" + Integer.toHexString(length));
        for (int offset = 0; offset < length && !monitor.isCancelled(); offset += 8) {
            Address address = start.add(offset);
            int count = Math.min(8, length - offset);
            byte[] bytes = new byte[count];
            currentProgram.getMemory().getBytes(address, bytes);
            StringBuilder hex = new StringBuilder();
            long value = 0;
            for (int i = 0; i < count; ++i) {
                hex.append(String.format("%02x", bytes[i] & 0xff));
                if (i + 1 != count) hex.append(' ');
                value |= ((long)bytes[i] & 0xffL) << (8 * i);
            }
            String pointerInfo = "";
            if (count == 8) {
                Address pointer = toAddr(value);
                if (currentProgram.getMemory().contains(pointer)) {
                    pointerInfo = " ptr=" + pointer + symbolAt(pointer);
                }
            }
            Data data = currentProgram.getListing().getDataContaining(address);
            Instruction instruction = currentProgram.getListing().getInstructionContaining(address);
            lines.add(address + ": " + hex + symbolAt(address) + pointerInfo +
                (data == null ? "" : " data=" + data) +
                (instruction == null ? "" : " insn=" + instruction));
            ReferenceIterator refs = currentProgram.getReferenceManager().getReferencesTo(address);
            while (refs.hasNext()) {
                Reference ref = refs.next();
                Function function = getFunctionContaining(ref.getFromAddress());
                lines.add("  REF from=" + ref.getFromAddress() + " type=" +
                    ref.getReferenceType() + " function=" +
                    (function == null ? "<none>" : function.getEntryPoint() + " " + function.getName()));
            }
        }
        Files.write(Path.of(args[0]), lines);
        for (String line : lines) println(line);
    }
}
