// Read-only diagnosis of a decompiler signature/calling-convention ambiguity.
import ghidra.app.script.GhidraScript;
import java.nio.file.Files;
import java.nio.file.Path;
public class DumpFunctionParameterStorage extends GhidraScript {
  protected void run() throws Exception {
    String[] args=getScriptArgs();
    var out=new StringBuilder("Target SHA256: "+currentProgram.getExecutableSHA256()+"\n");
    for (int i=1;i<args.length;i++) {
      var f=getFunctionAt(toAddr(args[i]));
      if (f==null) { out.append("No function at "+args[i]+"\n"); continue; }
      out.append("\n"+f.getEntryPoint()+" "+f.getName()+"\n");
      out.append("Signature: "+f.getSignature()+" convention="+f.getCallingConventionName()+" customStorage="+f.hasCustomVariableStorage()+" callFixup="+f.getCallFixup()+"\n");
      for (var p:f.getParameters()) out.append(p.getOrdinal()+" "+p.getName()+" "+p.getDataType()+" storage="+p.getVariableStorage()+"\n");
    }
    Files.writeString(Path.of(args[0]),out.toString());
  }
}
