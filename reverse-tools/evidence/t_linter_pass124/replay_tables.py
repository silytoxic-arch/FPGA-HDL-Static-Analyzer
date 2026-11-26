from pathlib import Path
import struct, json, hashlib
import argparse
cli=argparse.ArgumentParser(description="Replay retained T-Linter lexer/LALR tables offline; never executes the binary")
cli.add_argument("--target",required=True)
cli.add_argument("--output",required=True)
args=cli.parse_args()
p=Path(args.target);d=p.read_bytes();assert hashlib.sha256(d).hexdigest()=="60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0", "Target hash mismatch";pe=struct.unpack_from('<I',d,0x3c)[0];n=struct.unpack_from('<H',d,pe+6)[0];opt=struct.unpack_from('<H',d,pe+20)[0];sections=[]
for i in range(n):
    at=pe+24+opt+i*40;vs,rva,size,off=struct.unpack_from('<IIII',d,at+8);sections.append((rva,size,off))
def offset(va):
    r=va-0x180000000
    return next(off+r-base for base,size,off in sections if base<=r<base+size)
def byte(va):return d[offset(va)]
def short(va):return struct.unpack_from('<h',d,offset(va))[0]
def candidates(word):
    state=1;history=[1]
    for ch in (word+' ').encode('ascii'):
        cc=byte(0x180e43140+ch)
        for _ in range(128):
            index=short(0x180e43290+state*2)+cc
            if short(0x180e465c0+index*2)==state:break
            state=short(0x180e43a40+state*2)
            if state>0x3b3:cc=byte(0x180e43240+cc)
        else:raise RuntimeError('DFA fallback loop')
        state=short(0x180e441f0+(short(0x180e43290+state*2)+cc)*2);history.append(state)
        if short(0x180e43290+state*2)==0x119e:break
    out=[]
    for end in range(len(history)-1,0,-1):
        state=history[end];lo=short(0x180e429d0+state*2);hi=short(0x180e429d0+(state+1)*2)
        if lo>0 and hi>lo:
            out.append({'length':end,'state':state,'actions':[short(0x180e41900+i*2) for i in range(lo,hi)]})
            break
    return out
def name(va):
    off=offset(va);return d[off:d.find(b'\0',off)].decode('ascii')
names=[]
for i in range(485):names.append(name(struct.unpack_from('<Q',d,offset(0x180f5c210)+8*i)[0]))
def parser(words):
    stream=[names.index(w) for w in words];pos=0;stack=[0];symbols=[];events=[];cached=None;last_read=-1
    for _ in range(4096):
        state=stack[-1]
        if state==3:return {'accepted':True,'reductions':events}
        pact=short(0x180f5d140+state*2)
        if pact!=-0x423 and cached is None:cached=stream[pos];last_read=pos
        token=stream[pos];index=pact+token
        if pact==-0x423 or index<0 or index>0xad2 or short(0x180f600f0+index*2)!=token:
            rule=short(0x180f5dbd0+state*2)
        else:
            action=short(0x180f5eb40+index*2)
            if action>0:
                stack.append(action);symbols.append(names[token]);pos+=1;cached=None;continue
            rule=-action
        if rule<=0:raise RuntimeError(f'parse error state={state} token={names[token]}')
        length=byte(0x180f626b0+rule);lhs=short(0x180f62130+rule*2);rhs=symbols[-length:] if length else []
        events.append({'rule':rule,'hex':hex(rule),'lhs':names[lhs],'rhs':rhs,'lastReadTokenIndex':last_read,'lastReadToken':words[last_read] if last_read>=0 else None})
        if length:del stack[-length:];del symbols[-length:]
        index=short(0x180f5e660+(lhs-179)*2)+stack[-1]
        state=short(0x180f5eb40+index*2) if 0<=index<=0xad2 and short(0x180f600f0+index*2)==stack[-1] else short(0x180f5e8d0+(lhs-179)*2)
        stack.append(state);symbols.append(names[lhs])
    raise RuntimeError('parse limit')
fixtures=[]
for spelling,kind in [('$clog2','TOK_ID'),('$signed','TOK_TO_SIGNED'),('$unsigned','TOK_TO_UNSIGNED')]:
    words=['TOK_MODULE','TOK_ID',"';'",'TOK_LOCALPARAM','TOK_ID',"'='",kind,"'('",'TOK_CONSTVAL',"')'","';'",'TOK_ENDMODULE','$end']
    result=parser(words);fixtures.append({'source':f'module m; localparam P={spelling}(2); endmodule','tokens':words,**result})
    print(spelling,[(e['hex'],e['lhs'],e['rhs'],e['lastReadToken']) for e in result['reductions'] if e['rule'] in [0x14e,0x152,0x288,0x289,0x28a,0x28b]])
for spelling,kind in [("signed", "TOK_SIGNED"),("unsigned", "TOK_UNSIGNED")]:
    words=['TOK_MODULE','TOK_ID',"';'",'TOK_LOCALPARAM','TOK_ID',"'='",kind,'OP_CAST',"'('",'TOK_CONSTVAL',"')'","';'",'TOK_ENDMODULE','$end']
    result=parser(words);fixtures.append({'source':f"module m; localparam P={spelling}'(2); endmodule",'tokens':words,**result})
    print(spelling,[(e['hex'],e['lhs'],e['rhs'],e['lastReadToken']) for e in result['reductions'] if e['rule'] in [0x2b4,0x2b5,0x152]])

out={'targetSha256':hashlib.sha256(d).hexdigest(),'scope':'Offline LALR grammar-table replay; predefined token kinds; no target execution, no semantic actions or full runtime','fixtures':fixtures}
out['tokenBindings']=[{'externalToken':hex(t),'symbolIndex':byte(0x180f5baf0+t),'symbol':names[byte(0x180f5baf0+t)]} for t in [0x103,0x104,0x11b,0x12e,0x158,0x159,0x16b,0x195]]
out['lexerProbes']=[{'input':w,'accepts':candidates(w)} for w in ['$clog2','$signed','$unsigned','signed','unsigned',"'",'localparam','CLK','P']]
assert all(f['accepted'] for f in fixtures)
Path(args.output).write_text(json.dumps(out,indent=2)+'\n')
