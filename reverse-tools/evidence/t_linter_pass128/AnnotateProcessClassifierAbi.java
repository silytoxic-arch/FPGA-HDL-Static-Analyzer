// Apply only register-verified ABI annotations to the analysis database, not the target binary.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function.FunctionUpdateType;
import ghidra.program.model.listing.ParameterImpl;
import ghidra.program.model.data.PointerDataType;
import ghidra.program.model.data.UnsignedIntegerDataType;
import ghidra.program.model.data.VoidDataType;
import ghidra.program.model.symbol.SourceType;
import java.nio.file.Files;
import java.nio.file.Path;
public class AnnotateProcessClassifierAbi extends GhidraScript {
  protected void run() throws Exception {
    if (!currentProgram.getExecutableSHA256().equals("60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0"))
      throw new IllegalArgumentException("Unexpected target SHA256; refusing ABI annotations");
    var out=new StringBuilder("Target SHA256: "+currentProgram.getExecutableSHA256()+"\nABI annotations inferred from retained instruction operands, not vendor debug symbols.\n");
    String cc=currentProgram.getCompilerSpec().getDefaultCallingConvention().getName();
    int transaction=currentProgram.startTransaction("Register-verified process classifier ABI");
    boolean success=false;
    try {
      for (String address:new String[]{"1804c5070","1804d5520"}) {
        var f=getFunctionAt(toAddr(address));
        f.setCallingConvention(cc);
        f.replaceParameters(FunctionUpdateType.DYNAMIC_STORAGE_ALL_PARAMS,true,SourceType.USER_DEFINED,
          new ParameterImpl("root",PointerDataType.dataType,currentProgram),
          new ParameterImpl(address.equals("1804c5070")?"source_if_nodes":"classification",PointerDataType.dataType,currentProgram));
        f.setReturnType(address.equals("1804c5070")?VoidDataType.dataType:UnsignedIntegerDataType.dataType,SourceType.USER_DEFINED);
        out.append(f.getEntryPoint()+" "+f.getSignature()+"\n");
        for (var p:f.getParameters()) out.append(p.getName()+" storage="+p.getVariableStorage()+"\n");
      }
      success=true;
    } finally { currentProgram.endTransaction(transaction,success); }
    Files.writeString(Path.of(getScriptArgs()[0]),out.toString());
  }
}
