from pathlib import Path
import struct, json, hashlib, sys
p=Path(sys.argv[1]);d=p.read_bytes();pe=struct.unpack_from('<I',d,0x3c)[0];n=struct.unpack_from('<H',d,pe+6)[0];opt=struct.unpack_from('<H',d,pe+20)[0];sections=[]
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
    stream=[names.index(w) for w in words];pos=0;stack=[0];nodes=[];events=[];cached=None;last_read=-1
    for _ in range(4096):
        state=stack[-1]
        if state==3:
            exprs=[e['node'] for e in events if e['lhs']=='expr']
            root=max(exprs,key=lambda n:n['end']-n['begin'])
            return {'accepted':True,'expressionDerivation':root,'assignmentReductions':[dict(rule=e['rule'],lhs=e['lhs'],rhs=e['rhs']) for e in events if 'OP_LE' in e['rhs']]}
        pact=short(0x180f5d140+state*2)
        if pact!=-0x423 and cached is None:cached=stream[pos];last_read=pos
        token=stream[pos];index=pact+token
        if pact==-0x423 or index<0 or index>0xad2 or short(0x180f600f0+index*2)!=token:
            rule=short(0x180f5dbd0+state*2)
        else:
            action=short(0x180f5eb40+index*2)
            if action>0:
                stack.append(action);nodes.append({'symbol':names[token],'begin':pos,'end':pos+1});pos+=1;cached=None;continue
            rule=-action
        if rule<=0:raise RuntimeError(f'parse error state={state} token={names[token]}')
        length=byte(0x180f626b0+rule);lhs=short(0x180f62130+rule*2);children=nodes[-length:] if length else []
        node={'rule':rule,'symbol':names[lhs],'begin':children[0]['begin'] if children else pos,'end':children[-1]['end'] if children else pos,'children':children}
        events.append({'rule':rule,'lhs':names[lhs],'rhs':[n['symbol'] for n in children],'node':node})
        if length:del stack[-length:];del nodes[-length:]
        index=short(0x180f5e660+(lhs-179)*2)+stack[-1]
        state=short(0x180f5eb40+index*2) if 0<=index<=0xad2 and short(0x180f600f0+index*2)==stack[-1] else short(0x180f5e8d0+(lhs-179)*2)
        stack.append(state);nodes.append(node)
    raise RuntimeError('parse limit')

# Compact derivation retains native rule numbers and meaningful terminals;
# one-child wrappers and empty attributes are omitted only from display.
def compact(node):
    if 'children' not in node:return node['symbol']+'@'+str(node['begin'])
    if not node['children']:return None
    children=[v for child in node['children'] if (v:=compact(child)) is not None]
    if len(children)==1:return children[0]
    return {'rule':node['rule'],'lhs':node['symbol'],'children':children}
fixtures=[]
streams=[
 ('logical_prefix_add',["'!'",'TOK_ID',"'+'",'TOK_ID']),
 ('logical_outer_add',["'!'","'('",'TOK_ID',"'+'",'TOK_ID',"')'"]),
 ('equality_then_or',['TOK_ID','OP_EQ','TOK_ID',"'|'",'TOK_ID']),
 ('power_chain',['TOK_ID','OP_POW','TOK_ID','OP_POW','TOK_ID'])]
for name,expression in streams:
    words=['TOK_MODULE','TOK_ID',"';'",'TOK_ASSIGN','TOK_ID',"'='"]+expression+["';'",'TOK_ENDMODULE','$end']
    result=parser(words)
    fixtures.append({'name':name,'tokens':words,'accepted':result['accepted'],'derivation':compact(result['expressionDerivation'])})
words=['TOK_MODULE','TOK_ID',"';'",'TOK_ALWAYS',"'@'","'*'",'TOK_ID','OP_LE','TOK_ID',"'?'",'TOK_CONSTVAL',"':'",'TOK_CONSTVAL',"';'",'TOK_ENDMODULE','$end']
result=parser(words)
fixtures.append({'name':'nonblocking_ternary','tokens':words,'accepted':result['accepted'],'derivation':compact(result['expressionDerivation']),'assignmentReductions':result['assignmentReductions']})
out={'targetSha256':hashlib.sha256(d).hexdigest(),'scope':'Offline native grammar-table replay; predefined token kinds, no target lexer/semantic execution. Derivation trees prove grouping, not width results.','fixtures':fixtures}
assert out['targetSha256']=='60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0'
Path(sys.argv[2]).write_text(json.dumps(out,indent=2)+'\n')
