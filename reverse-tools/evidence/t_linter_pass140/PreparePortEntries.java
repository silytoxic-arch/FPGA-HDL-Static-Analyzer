import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
public class PreparePortEntries extends GhidraScript {
    protected void run() throws Exception {
        if (!"60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0".equals(currentProgram.getExecutableSHA256()))
            throw new IllegalArgumentException("Unexpected target digest");
        for (String text : getScriptArgs()) {
            Address entry=toAddr(text);
            disassemble(entry);
            if (getFunctionAt(entry)==null) createFunction(entry,null);
            println("Prepared explicit entry "+entry+"; no full-program analysis requested");
        }
    }
}
