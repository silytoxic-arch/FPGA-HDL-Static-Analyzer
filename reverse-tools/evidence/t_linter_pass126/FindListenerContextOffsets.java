// Enumerate named VHDL parser context callback instructions for selected listener offsets.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.scalar.Scalar;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.HashSet;
public class FindListenerContextOffsets extends GhidraScript {
  protected void run() throws Exception {
    String[] args = getScriptArgs();
    var wanted = new HashSet<Long>();
    for (int i=1;i<args.length;i++) wanted.add(Long.parseUnsignedLong(args[i],16));
    var out = new StringBuilder("Target SHA256: "+currentProgram.getExecutableSHA256()+"\n");
    var symbols = currentProgram.getSymbolTable().getAllSymbols(true);
    int contexts=0;
    while (symbols.hasNext()) {
      monitor.checkCancelled();
      var symbol=symbols.next(); var name=symbol.getName(true);
      if (!name.contains("Vhdl2008ParserParser::") || !name.endsWith("::vftable")) continue;
      contexts++;
      for (int slot: new int[]{0x80,0x88}) {
        var callback=toAddr(currentProgram.getMemory().getLong(symbol.getAddress().add(slot)));
        var f=getFunctionAt(callback); if (f==null) continue;
        var instructions=currentProgram.getListing().getInstructions(f.getBody(),true);
        while (instructions.hasNext()) {
          var ins=instructions.next();
          for (int operand=0;operand<ins.getNumOperands();operand++)
            for (var obj: ins.getOpObjects(operand))
              if (obj instanceof Scalar && wanted.contains(((Scalar)obj).getUnsignedValue()))
                out.append(name+" vtable="+symbol.getAddress()+" contextSlot="+Integer.toHexString(slot)+" callback="+callback+" instruction="+ins.getAddress()+" "+ins+"\n");
        }
      }
    }
    out.append("Scanned named contexts: "+contexts+"; direct displacement matches only, not complete provenance.\n");
    Files.writeString(Path.of(args[0]),out.toString());
  }
}
