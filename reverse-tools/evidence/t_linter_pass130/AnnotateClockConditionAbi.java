// Analysis-only ABI inferred from class2 call instructions; no vendor symbols.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function.FunctionUpdateType;
import ghidra.program.model.listing.ParameterImpl;
import ghidra.program.model.data.PointerDataType;
import ghidra.program.model.data.UnsignedCharDataType;
import ghidra.program.model.symbol.SourceType;
import java.nio.file.Files;
import java.nio.file.Path;
public class AnnotateClockConditionAbi extends GhidraScript {
  protected void run() throws Exception {
    if (!currentProgram.getExecutableSHA256().equals("60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0"))
      throw new IllegalArgumentException("Unexpected target SHA256");
    String[] sites={"1804d696c","1804d6971","1804d6975","1804d6978","1804d697d"};
    String[] required={"LEA R8,","LEA RDX,","MOV RCX,R14","CALL 0x1804c4430","TEST AL,AL"};
    for (int i=0;i<sites.length;++i) {
      var instruction=getInstructionAt(toAddr(sites[i]));
      if (instruction==null || !instruction.toString().startsWith(required[i]))
        throw new IllegalArgumentException("ABI call-site guard failed at "+sites[i]);
    }
    var out=new StringBuilder("Target SHA256: "+currentProgram.getExecutableSHA256()+"\nInferred from class2 call-site RCX/RDX/R8 and AL use, not vendor debug symbols. Only analysis database updated.\n");
    var f=getFunctionAt(toAddr("1804c4430"));
    f.setCallingConvention(currentProgram.getCompilerSpec().getDefaultCallingConvention().getName());
    f.replaceParameters(FunctionUpdateType.DYNAMIC_STORAGE_ALL_PARAMS,true,SourceType.USER_DEFINED,
      new ParameterImpl("condition",PointerDataType.dataType,currentProgram),
      new ParameterImpl("clock_name",PointerDataType.dataType,currentProgram),
      new ParameterImpl("direction",PointerDataType.dataType,currentProgram));
    f.setReturnType(UnsignedCharDataType.dataType,SourceType.USER_DEFINED);
    out.append(f.getSignature()+"\n");
    for (var p:f.getParameters()) out.append(p.getName()+" storage="+p.getVariableStorage()+"\n");
    Files.writeString(Path.of(getScriptArgs()[0]),out.toString());
  }
}
