
// =============================================================================
//  FX Benchmark Library — 用于测试 Slang 模块编译性能
// =============================================================================

static const float FX_PI = 3.141592653589793;
static const float FX_TAU = 6.283185307179586;
static const float FX_EPS = 1e-6;

// ==== 基础数学 (最先, 无依赖) ===============================================
float fx_sat(float x)            { return clamp(x,0,1); }
float2 fx_sat(float2 x)          { return clamp(x,0,1); }
float3 fx_sat(float3 x)          { return clamp(x,0,1); }
float fx_rcp(float x)            { return 1.0/max(x,1e-6); }
float fx_pow2(float x)           { return x*x; }
float fx_pow3(float x)           { return x*x*x; }
float fx_pow5(float x)           { float x2=x*x; return x2*x2*x; }
float fx_luminance(float3 c)     { return dot(c,float3(0.2126,0.7152,0.0722)); }
float fx_radians(float d)        { return d*FX_PI/180.0; }
float fx_smooth(float a,float b,float t){ float x=clamp((t-a)/(b-a),0,1); return x*x*(3-2*x); }
float fx_smoother(float a,float b,float t){ float x=clamp((t-a)/(b-a),0,1); return x*x*x*(x*(x*6-15)+10); }
float fx_hash2D(float2 p){ return frac(sin(dot(p,float2(127.1,311.7)))*43758.5453); }
float fx_hash3D(float3 p){ return frac(sin(dot(p,float3(127.1,311.7,74.7)))*43758.5453); }

// ndot — SDF 2D 辅助, 需在所有 SDF 之前定义
float ndot(float2 a,float2 b){ return a.x*b.x-a.y*b.y; }

// ==== SDF 2D 形状 (40个) ====================================================
float fx_sdCircle(float2 p,float r){ return length(p)-r; }
float fx_sdBox(float2 p,float2 b){ float2 d=abs(p)-b; return length(max(d,0))+min(max(d.x,d.y),0); }
float fx_sdRoundBox(float2 p,float2 b,float r){ float2 d=abs(p)-b+r; return length(max(d,0))+min(max(d.x,d.y),0)-r; }
float fx_sdTriangle(float2 p,float2 a,float2 b,float2 c){ float2 e0=b-a,e1=c-b,e2=a-c; float2 v0=p-a,v1=p-b,v2=p-c; float2 pq0=v0-e0*clamp(dot(v0,e0)/dot(e0,e0),0,1); float2 pq1=v1-e1*clamp(dot(v1,e1)/dot(e1,e1),0,1); float2 pq2=v2-e2*clamp(dot(v2,e2)/dot(e2,e2),0,1); float s=sign(e0.x*e2.y-e0.y*e2.x); float2 d=min(min(float2(dot(pq0,pq0),s*(v0.x*e0.y-v0.y*e0.x)),float2(dot(pq1,pq1),s*(v1.x*e1.y-v1.y*e1.x))),float2(dot(pq2,pq2),s*(v2.x*e2.y-v2.y*e2.x))); return -sqrt(d.x)*sign(d.y); }
float fx_sdEquilateralTriangle(float2 p,float r){ const float k=sqrt(3.0); p.x=abs(p.x)-r; p.y=p.y+r/k; if(p.x+k*p.y>0)p=float2(p.x-k*p.y,-k*p.x-p.y)/2.0; p.x-=clamp(p.x,-2.0*r,0); return -length(p)*sign(p.y); }
float fx_sdIsoscelesTriangle(float2 p,float2 q){ p.x=abs(p.x); float2 a=p-q*clamp(dot(p,q)/dot(q,q),0,1); float2 b=p-q*float2(clamp(p.x/q.x,0,1),1); float s=-sign(q.y); float2 d=min(float2(dot(a,a),s*(p.x*q.y-p.y*q.x)),float2(dot(b,b),s*(p.x*q.y-p.y*q.x))); return -sqrt(d.x)*sign(d.y); }
float fx_sdRhombus(float2 p,float2 b){ p=abs(p); float h=clamp((-2.0*ndot(p,b)+ndot(b,b))/dot(b,b),-1,1); float d=length(p-0.5*float2(1-h,1+h)*b); return d*sign(p.x*b.y+p.y*b.x-b.x*b.y); }
float fx_sdTrapezoid(float2 p,float r1,float r2,float h){ float2 k1=float2(r2,h),k2=float2(r2-r1,2.0*h); p.x=abs(p.x); float2 ca=float2(p.x-min(p.x,(p.y<0)?r1:r2),abs(p.y)-h); float2 cb=p-k1+k2*clamp(dot(k1-p,k2)/dot(k2,k2),0,1); float s=(p.x<0&&p.y<0)?-1:1; return s*sqrt(min(dot(ca,ca),dot(cb,cb))); }
float fx_sdPentagon(float2 p,float r){ const float3 k=float3(0.809016994,0.587785252,0.726542528); p.x=abs(p.x); p-=2.0*min(dot(float2(-k.x,k.y),p),0)*float2(-k.x,k.y); p-=2.0*min(dot(float2(k.x,k.y),p),0)*float2(k.x,k.y); p-=float2(clamp(p.x,-r*k.z,r*k.z),r); return length(p)*sign(p.y); }
float fx_sdHexagon(float2 p,float r){ const float3 k=float3(-0.866025404,0.5,0.577350269); p=abs(p); p-=2.0*min(dot(k.xy,p),0)*k.xy; p-=float2(clamp(p.x,-k.z*r,k.z*r),r); return length(p)*sign(p.y); }
float fx_sdOctagon(float2 p,float r){ const float3 k=float3(-0.9238795325,0.3826834323,0.4142135623); p=abs(p); p-=2.0*min(dot(float2(k.x,k.y),p),0)*float2(k.x,k.y); p-=2.0*min(dot(float2(-k.x,k.y),p),0)*float2(-k.x,k.y); p-=float2(clamp(p.x,-k.z*r,k.z*r),r); return length(p)*sign(p.y); }
float fx_sdStar(float2 p,float r,int n,float m){ float an=FX_PI/float(n); float en=FX_PI/m; float2 acs=float2(cos(an),sin(an)); float2 ecs=float2(cos(en),sin(en)); float bn=fmod(atan2(p.y,p.x),2.0*an)-an; p=length(p)*float2(cos(bn),abs(sin(bn))); p-=r*acs; p+=ecs*clamp(-dot(p,ecs),0,r*acs.y/ecs.y); return length(p)*sign(p.x); }
float fx_sdStar5(float2 p,float r,float rf){ const float2 k1=float2(0.809016994375,-0.587785252292); const float2 k2=float2(-k1.x,k1.y); p.x=abs(p.x); p-=2.0*max(dot(k1,p),0)*k1; p-=2.0*max(dot(k2,p),0)*k2; p.x=abs(p.x); p.y-=r; float2 ba=rf*float2(-k1.y,k1.x)-float2(0,1); float h=clamp(dot(p,ba)/dot(ba,ba),0,r); return length(p-ba*h)*sign(p.y*ba.x-p.x*ba.y); }
float fx_sdCross(float2 p,float2 b,float r){ p=abs(p); p=(p.y>p.x)?p.yx:p.xy; float2 q=p-b.xy; float k=max(q.y,q.x); float2 w=(k>0)?q:float2(b.y-p.x,-k); return sign(k)*length(max(w,0))+r; }
float fx_sdRoundedX(float2 p,float w,float r){ p=abs(p); return length(p-min(p.x+p.y,w)*0.5)-r; }
float fx_sdArc(float2 p,float2 sca,float r){ p.x=abs(p.x); float k=(sca.y*p.x>sca.x*p.y)?dot(p.xy,sca):length(p.xy); return sqrt(dot(p,p)+r*r-2.0*r*k); }
float fx_sdMoon(float2 p,float d,float ra,float rb){ p.y=abs(p.y); float a=(ra*ra-rb*rb+d*d)/(2.0*d); float b=sqrt(max(ra*ra-a*a,0)); float dist=(d*abs(p.y)<b*p.x)?length(p)-ra:length(p-float2(a,b))-rb; return dist; }
float fx_sdVesica(float2 p,float r,float d){ p=abs(p); float b=sqrt(r*r-d*d); return ((p.y-b)*d>p.x*b)?length(p-float2(0,b)):length(p-float2(-d,0))-r; }
float fx_sdHeart(float2 p){ p.x=abs(p.x); if(p.y+p.x>1.0)return sqrt(dot(p-float2(0.25,0.75),p-float2(0.25,0.75)))-sqrt(2.0)/4.0; return sqrt(min(dot(p-float2(0.00,0.45),p-float2(0.00,0.45)),dot(p-float2(0.75,0.45),p-float2(0.75,0.45))))*sign(p.x-p.y); }
float fx_sdPie(float2 p,float2 c,float r){ p.x=abs(p.x); float l=length(p)-r; float m=length(p-c*clamp(dot(p,c),0,r)); return max(l,m*sign(c.y*p.x-c.x*p.y)); }
float fx_sdEllipse(float2 p,float2 r){ return(length(p/r)-1.0)*min(min(r.x,r.y),length(p)); }
float fx_sdLine(float2 p,float2 a,float2 b){ float2 pa=p-a,ba=b-a; float h=clamp(dot(pa,ba)/dot(ba,ba),0,1); return length(pa-ba*h); }
float fx_sdSegment(float2 p,float2 a,float2 b){ float2 pa=p-a,ba=b-a; float h=clamp(dot(pa,ba)/dot(ba,ba),0,1); return length(pa-ba*h); }
float fx_sdFlatRing(float2 p,float ra,float rb){ return abs(length(p)-0.5*(ra+rb))-0.5*(rb-ra); }

// ==== SDF 3D 形状 (30个) ====================================================
float fx_sdSphere(float3 p,float r){ return length(p)-r; }
float fx_sdBox3D(float3 p,float3 b){ float3 q=abs(p)-b; return length(max(q,0))+min(max(q.x,max(q.y,q.z)),0); }
float fx_sdRoundBox3D(float3 p,float3 b,float r){ float3 q=abs(p)-b+r; return length(max(q,0))+min(max(q.x,max(q.y,q.z)),0)-r; }
float fx_sdTorus(float3 p,float2 t){ float2 q=float2(length(p.xz)-t.x,p.y); return length(q)-t.y; }
float fx_sdCappedTorus(float3 p,float2 sc,float ra,float rb){ p.x=abs(p.x); float k=(sc.y*p.x>sc.x*p.y)?dot(p.xy,sc):length(p.xy); return sqrt(dot(p,p)+ra*ra-2.0*ra*k)-rb; }
float fx_sdLink(float3 p,float le,float r1,float r2){ float3 q=float3(p.x,max(abs(p.y)-le,0),p.z); return length(float2(length(q.xy)-r1,q.z))-r2; }
float fx_sdCylinder(float3 p,float2 h){ float2 d=abs(float2(length(p.xz),p.y))-h; return min(max(d.x,d.y),0)+length(max(d,0)); }
float fx_sdCappedCylinder(float3 p,float h,float r){ float2 d=abs(float2(length(p.xz),p.y))-float2(r,h); return min(max(d.x,d.y),0)+length(max(d,0)); }
float fx_sdRoundedCylinder(float3 p,float ra,float rb,float h){ float2 d=float2(length(p.xz)-2.0*ra+rb,abs(p.y)-h); return min(max(d.x,d.y),0)+length(max(d,0))-rb; }
float fx_sdCone(float3 p,float2 c,float h){ float2 q=h*float2(c.x/c.y,-1); float2 w=float2(length(p.xz),p.y); float2 a=w-q*clamp(dot(w,q)/dot(q,q),0,1); float2 b=w-q*float2(clamp(w.x/q.x,0,1),1); float k=sign(q.y); float2 d=min(float2(dot(a,a),k*(w.x*q.y-w.y*q.x)),float2(dot(b,b),k*(w.x*q.y-w.y*q.x))); return sqrt(d.x)*sign(d.y); }
float fx_sdInfiniteCone(float3 p,float2 c){ float2 q=float2(length(p.xz),-p.y); float d=length(q-c*max(dot(q,c),0)); return d*((q.x*c.y-q.y*c.x<0)?-1:1); }
float fx_sdPlane(float3 p,float3 n,float h){ return dot(p,n)+h; }
float fx_sdPrism(float3 p,float2 h){ float3 q=abs(p); return max(q.z-h.y,max(q.x*0.866025+p.y*0.5,-p.y)-h.x*0.5); }
float fx_sdCapsule(float3 p,float3 a,float3 b,float r){ float3 pa=p-a,ba=b-a; float h=clamp(dot(pa,ba)/dot(ba,ba),0,1); return length(pa-ba*h)-r; }
float fx_sdEllipsoid3D(float3 p,float3 r){ float k0=length(p/r); float k1=length(p/(r*r)); return k0*(k0-1.0)/k1; }
float fx_sdOctahedron(float3 p,float s){ p=abs(p); float m=p.x+p.y+p.z-s; float3 q; if(3.0*p.x<m)q=p.xyz; else if(3.0*p.y<m)q=p.yzx; else if(3.0*p.z<m)q=p.zxy; else return m*0.57735027; float k=clamp(0.5*(q.z-q.y+s),0,s); return length(float3(q.x,q.y-s+k,q.z-k)); }
float fx_sdDodecahedron(float3 p,float r){ float3 a=abs(p); float d1=a.x*0.8506508+a.y*0.5257311; float d2=a.y*0.8506508+a.z*0.5257311; float d3=a.z*0.8506508+a.x*0.5257311; return(max(max(d1,d2),d3)-r)*0.995; }

// ==== SDF 布尔操作 & 变换 (18个) =============================================
float fx_opUnion(float d1,float d2){ return min(d1,d2); }
float fx_opSubtraction(float d1,float d2){ return max(-d1,d2); }
float fx_opIntersection(float d1,float d2){ return max(d1,d2); }
float fx_opXor(float d1,float d2){ return max(min(d1,d2),-max(d1,d2)); }
float fx_opSmoothUnion(float d1,float d2,float k){ float h=clamp(0.5+0.5*(d2-d1)/k,0,1); return lerp(d2,d1,h)-k*h*(1.0-h); }
float fx_opSmoothSubtraction(float d1,float d2,float k){ float h=clamp(0.5-0.5*(d2+d1)/k,0,1); return lerp(d2,-d1,h)+k*h*(1.0-h); }
float fx_opSmoothIntersection(float d1,float d2,float k){ float h=clamp(0.5-0.5*(d2-d1)/k,0,1); return lerp(d2,d1,h)+k*h*(1.0-h); }
float fx_opRound(float d,float r){ return d-r; }
float fx_opOnion(float d,float t){ return abs(d)-t; }

// ==== Noise 函数 (12个) ====================================================
float fx_perlin2D(float2 p){ float2 i=floor(p); float2 f=frac(p); float2 u=f*f*(3.0-2.0*f); return lerp(lerp(fx_hash2D(i),fx_hash2D(i+float2(1,0)),u.x),lerp(fx_hash2D(i+float2(0,1)),fx_hash2D(i+1),u.x),u.y); }
float fx_perlin3D(float3 p){ float3 i=floor(p); float3 f=frac(p); float3 u=f*f*(3.0-2.0*f); float2 a=float2(0,0); float2 b=float2(1,0); float2 c=float2(0,1); float2 d=float2(1,1); return lerp(lerp(lerp(fx_hash2D(i.xy+a),fx_hash2D(i.xy+b),u.x),lerp(fx_hash2D(i.xy+c),fx_hash2D(i.xy+d),u.x),u.y),lerp(lerp(fx_hash2D((i+1).xy+a),fx_hash2D((i+1).xy+b),u.x),lerp(fx_hash2D((i+1).xy+c),fx_hash2D((i+1).xy+d),u.x),u.y),u.z); }
float fx_fbm2D(float2 p,int oct){ float val=0; float amp=0.5; float freq=1.0; for(int i=0;i<8;i++){ if(i>=oct)break; val+=amp*fx_perlin2D(p*freq); freq*=2.0; amp*=0.5; } return val; }
float fx_fbm3D(float3 p,int oct){ float val=0; float amp=0.5; float freq=1.0; for(int i=0;i<8;i++){ if(i>=oct)break; val+=amp*fx_perlin3D(p*freq); freq*=2.0; amp*=0.5; } return val; }
float fx_worley2D(float2 p){ float2 i=floor(p); float2 f=frac(p); float md=8.0; for(int y=-1;y<=1;y++)for(int x=-1;x<=1;x++){ float2 g=float2(x,y); float2 r=float2(fx_hash2D(i+g),fx_hash2D(i+g+float2(31,17))); float2 d=g+r-f; md=min(md,dot(d,d)); } return sqrt(md); }
float fx_ridgedNoise2D(float2 p){ return 1.0-abs(fx_perlin2D(p))*2.0; }
float fx_domainWarp2D(float2 p){ float2 q=float2(fx_perlin2D(p),fx_perlin2D(p+float2(5.2,1.3))); float2 r=float2(fx_perlin2D(p+4.0*q+float2(1.7,9.2)),fx_perlin2D(p+4.0*q+float2(8.3,2.8))); return fx_perlin2D(p+4.0*r); }

// ==== Box Blur 变体 (7个, 循环生成) =========================================
float3 fx_boxBlur3(Texture2D tex,SamplerState samp,float2 uv,float2 ts){ float3 r=float3(0,0,0); for(int i=-1;i<=1;i++)for(int j=-1;j<=1;j++)r+=tex.Sample(samp,uv+float2(i,j)*ts).rgb; return r/9.0; }
float3 fx_boxBlur5(Texture2D tex,SamplerState samp,float2 uv,float2 ts){ float3 r=float3(0,0,0); for(int i=-2;i<=2;i++)for(int j=-2;j<=2;j++)r+=tex.Sample(samp,uv+float2(i,j)*ts).rgb; return r/25.0; }
float3 fx_boxBlur7(Texture2D tex,SamplerState samp,float2 uv,float2 ts){ float3 r=float3(0,0,0); for(int i=-3;i<=3;i++)for(int j=-3;j<=3;j++)r+=tex.Sample(samp,uv+float2(i,j)*ts).rgb; return r/49.0; }
float3 fx_boxBlur9(Texture2D tex,SamplerState samp,float2 uv,float2 ts){ float3 r=float3(0,0,0); for(int i=-4;i<=4;i++)for(int j=-4;j<=4;j++)r+=tex.Sample(samp,uv+float2(i,j)*ts).rgb; return r/81.0; }
float3 fx_boxBlur11(Texture2D tex,SamplerState samp,float2 uv,float2 ts){ float3 r=float3(0,0,0); for(int i=-5;i<=5;i++)for(int j=-5;j<=5;j++)r+=tex.Sample(samp,uv+float2(i,j)*ts).rgb; return r/121.0; }
float3 fx_boxBlur13(Texture2D tex,SamplerState samp,float2 uv,float2 ts){ float3 r=float3(0,0,0); for(int i=-6;i<=6;i++)for(int j=-6;j<=6;j++)r+=tex.Sample(samp,uv+float2(i,j)*ts).rgb; return r/169.0; }
float3 fx_boxBlur15(Texture2D tex,SamplerState samp,float2 uv,float2 ts){ float3 r=float3(0,0,0); for(int i=-7;i<=7;i++)for(int j=-7;j<=7;j++)r+=tex.Sample(samp,uv+float2(i,j)*ts).rgb; return r/225.0; }

// ==== 色彩操作 (30个) ========================================================
float3 fx_saturation(float3 c,float s){ float l=fx_luminance(c); return l+s*(c-l); }
float3 fx_vibrance(float3 c,float st){ float l=fx_luminance(c); float mx=max(max(c.r,c.g),c.b); float mn=min(min(c.r,c.g),c.b); float sat=mx-mn; return c+st*(1.0-sat)*(c-l); }
float3 fx_brightness(float3 c,float b){ return c+b; }
float3 fx_exposure(float3 c,float ev){ return c*exp2(ev); }
float3 fx_contrast(float3 c,float a){ return(c-0.5)*(1.0+a)+0.5; }
float3 fx_srgbToLinear(float3 c){ float3 r; r.r=(c.r<=0.04045)?c.r/12.92:pow((c.r+0.055)/1.055,2.4); r.g=(c.g<=0.04045)?c.g/12.92:pow((c.g+0.055)/1.055,2.4); r.b=(c.b<=0.04045)?c.b/12.92:pow((c.b+0.055)/1.055,2.4); return r; }
float3 fx_linearToSRGB(float3 c){ float3 r; r.r=(c.r<=0.0031308)?12.92*c.r:1.055*pow(c.r,1.0/2.4)-0.055; r.g=(c.g<=0.0031308)?12.92*c.g:1.055*pow(c.g,1.0/2.4)-0.055; r.b=(c.b<=0.0031308)?12.92*c.b:1.055*pow(c.b,1.0/2.4)-0.055; return r; }
float3 fx_reinhard(float3 c){ return c/(c+1.0); }
float3 fx_aces(float3 c){ float a=2.51,b=0.03,cc=2.43,d=0.59,e=0.14; return fx_sat((c*(a*c+b))/(c*(cc*c+d)+e)); }
float3 fx_hable(float3 c){ float A=0.22,B=0.30,C=0.10,D=0.20,E=0.01,F=0.30; return fx_sat(((c*(A*c+C*B)+D*E)/(c*(A*c+B)+D*F))-E/F); }
float3 fx_filmic(float3 c){ c=max(0,c-0.004); return(c*(6.2*c+0.5))/(c*(6.2*c+1.7)+0.06); }
float3 fx_lottes(float3 c){ float a=1.6,d=0.977; float b=(-pow(0.18,a)+pow(8.0,a)*0.267)/((pow(8.0,a*d)-pow(0.18,a*d))*0.267); float c0=(pow(8.0,a*d)*pow(0.18,a)-pow(8.0,a)*pow(0.18,a*d)*0.267)/((pow(8.0,a*d)-pow(0.18,a*d))*0.267); return fx_sat(pow(c,a)/(pow(c,a*d)*b+c0)); }
float3 fx_invert(float3 c){ return 1.0-c; }
float3 fx_grayscale(float3 c){ float l=fx_luminance(c); return float3(l,l,l); }
float3 fx_sepia(float3 c){ float l=fx_luminance(c); return float3(l*1.351,l*1.203,l*0.937); }
float3 fx_solarize(float3 c,float t){ return lerp(c,1.0-c,step(t,fx_luminance(c))); }
float3 fx_posterize(float3 c,float levels){ return floor(c*levels)/(levels-1.0); }
float3 fx_colorTemp(float3 c,float shift){ float3 warm=float3(1.0,0.78,0.50); float3 cool=float3(0.50,0.67,1.0); return c*lerp(cool,warm,shift*0.5+0.5); }
float3 fx_tealOrange(float3 c,float s){ float l=fx_luminance(c); return lerp(c,lerp(float3(0,0.5,0.5),float3(1,0.5,0),l),s); }
float3 fx_bleachBypass(float3 c,float op){ float l=fx_luminance(c); return lerp(c,fx_sat(2.0*c*float3(l,l,l)),op); }
float3 fx_crossProcess(float3 c){ c.r=fx_sat(c.r*1.2-0.1); c.g=fx_sat(c.g*0.9+0.05); c.b=fx_sat(c.b*1.1+0.05); return c; }
float3 fx_liftGammaGain(float3 c,float3 lift,float3 gamma,float3 gain){ return pow(fx_sat(gain*(c+lift)),1.0/gamma); }
float3 fx_hueShift(float3 c,float a){ float cA=cos(a),sA=sin(a); float3x3 rot=float3x3(cA+(1-cA)/3.0,(1-cA)/3.0-sqrt(1.0/3.0)*sA,(1-cA)/3.0+sqrt(1.0/3.0)*sA,(1-cA)/3.0+sqrt(1.0/3.0)*sA,cA+(1-cA)/3.0,(1-cA)/3.0-sqrt(1.0/3.0)*sA,(1-cA)/3.0-sqrt(1.0/3.0)*sA,(1-cA)/3.0+sqrt(1.0/3.0)*sA,cA+(1-cA)/3.0); return mul(rot,c); }
float3 fx_shadowsMidtonesHighlights(float3 c,float3 shad,float3 mid,float3 high){ float l=fx_luminance(c); float sh=1.0-smoothstep(0.0,0.4,l); float hi=smoothstep(0.6,1.0,l); float mi=1.0-sh-hi; return c+shad*sh+mid*mi+high*hi; }

// ==== HSV/HSL ================================================================
float3 fx_rgb2hsv(float3 c){ float4 K=float4(0.0,-1.0/3.0,2.0/3.0,-1.0); float4 p=lerp(float4(c.bg,K.wz),float4(c.gb,K.xy),step(c.b,c.g)); float4 q=lerp(float4(p.xyw,c.r),float4(c.r,p.yzx),step(p.x,c.r)); float d=q.x-min(q.w,q.y); return float3(abs(q.z+(q.w-q.y)/(6.0*d+1e-10)),d/(q.x+1e-10),q.x); }
float3 fx_hsv2rgb(float3 c){ float4 K=float4(1.0,2.0/3.0,1.0/3.0,3.0); float3 p=abs(frac(c.xxx+K.xyz)*6.0-K.www); return c.z*lerp(K.xxx,fx_sat(p-K.xxx),c.y); }

// ==== 混合模式 (25个) =======================================================
float3 fx_blendScreen(float3 a,float3 b){ return 1.0-(1.0-a)*(1.0-b); }
float3 fx_blendMultiply(float3 a,float3 b){ return a*b; }
float3 fx_blendOverlay(float3 a,float3 b){ bool3 c=a>0.5; return lerp(2.0*a*b,1.0-2.0*(1.0-a)*(1.0-b),c); }
float3 fx_blendSoftLight(float3 a,float3 b){ float3 lo=2.0*a*b+a*a*(1.0-2.0*b); float3 hi=2.0*a*(1.0-b)+sqrt(a)*(2.0*b-1.0); return lerp(hi,lo,b<0.5); }
float3 fx_blendHardLight(float3 a,float3 b){ return fx_blendOverlay(b,a); }
float3 fx_blendLinearLight(float3 a,float3 b){ return a+2.0*b-1.0; }
float3 fx_blendVividLight(float3 a,float3 b){ bool3 c=b<0.5; float3 lo=1.0-(1.0-a)/(2.0*b+FX_EPS); float3 hi=a/(2.0*(1.0-b)+FX_EPS); return fx_sat(lerp(hi,lo,c)); }
float3 fx_blendPinLight(float3 a,float3 b){ bool3 c=b<0.5; return lerp(min(a,2.0*b),max(a,2.0*(b-0.5)),c); }
float3 fx_blendAdd(float3 a,float3 b){ return a+b; }
float3 fx_blendSubtract(float3 a,float3 b){ return a-b; }
float3 fx_blendDifference(float3 a,float3 b){ return abs(a-b); }
float3 fx_blendExclusion(float3 a,float3 b){ return a+b-2.0*a*b; }
float3 fx_blendColorDodge(float3 a,float3 b){ return a/(1.0-b+FX_EPS); }
float3 fx_blendColorBurn(float3 a,float3 b){ return 1.0-(1.0-a)/(b+FX_EPS); }
float3 fx_blendLighten(float3 a,float3 b){ return max(a,b); }
float3 fx_blendDarken(float3 a,float3 b){ return min(a,b); }
float3 fx_blendAverage(float3 a,float3 b){ return(a+b)*0.5; }
float3 fx_blendNegation(float3 a,float3 b){ return 1.0-abs(1.0-a-b); }
float3 fx_blendReflect(float3 a,float3 b){ return(a*a)/(1.0-b+FX_EPS); }
float3 fx_blendGlow(float3 a,float3 b){ return fx_blendReflect(b,a); }
float3 fx_blendPhoenix(float3 a,float3 b){ return min(a,b)-max(a,b)+1.0; }
float3 fx_blendDivide(float3 a,float3 b){ return a/(b+FX_EPS); }
float3 fx_blendGrainExtract(float3 a,float3 b){ return a-b+0.5; }
float3 fx_blendGrainMerge(float3 a,float3 b){ return a+b-0.5; }

// ==== 后处理效果 (18个) ======================================================
float fx_vignette(float2 uv,float intensity,float smooth){ uv=uv*2.0-1.0; float d=length(uv); return 1.0-smoothstep(intensity-smooth,intensity,d); }
float fx_vignetteSquare(float2 uv,float r,float f){ uv=uv*2.0-1.0; return 1.0-smoothstep(r,r-f,dot(uv,uv)); }
float fx_filmGrain(float2 uv,float t,float i){ return(fx_hash2D(uv+t)-0.5)*i; }
float fx_scanline(float2 uv,float freq,float duty){ return smoothstep(duty,duty+0.05,frac(uv.y*freq)); }
float3 fx_crtEffect(float3 c,float2 uv,float sf){ return c*(0.7+0.3*fx_scanline(uv,sf,0.7)); }
float2 fx_lensDistortion(float2 uv,float k){ uv-=0.5; float r2=dot(uv,uv); float f=1.0+k*r2; return uv*f+0.5; }
float2 fx_barrelDistortion(float2 uv,float k){ float2 d=uv-0.5; float r=dot(d,d); return 0.5+d*(1.0+k*r+k*k*r*r); }
float2 fx_pincushionDistortion(float2 uv,float k){ float2 d=uv-0.5; float r=dot(d,d); return 0.5+d/(1.0+k*r); }
float3 fx_glow(Texture2D tex,SamplerState samp,float2 uv,float2 ts,float r){ float3 c=float3(0,0,0); for(int i=-3;i<=3;i++)for(int j=-3;j<=3;j++){ float2 off=float2(i,j)*ts*r; c+=tex.Sample(samp,uv+off).rgb*exp(-dot(off,off)/(r*r)); } return c/49.0; }
float3 fx_bloom(Texture2D tex,SamplerState samp,float2 uv,float2 ts,float r,float thresh){ float3 c=tex.Sample(samp,uv).rgb; float3 b=float3(0,0,0); for(int i=-5;i<=5;i++)for(int j=-5;j<=5;j++){ float2 off=float2(i,j)*ts*r; float3 s=tex.Sample(samp,uv+off).rgb; b+=s*step(thresh,fx_luminance(s)); } return c+b/121.0; }
float3 fx_radialBlur(Texture2D tex,SamplerState samp,float2 uv,float2 ts,float2 center,float str){ float2 dir=uv-center; float3 c=float3(0,0,0); for(int i=0;i<16;i++){ float t=(float(i)/15.0-0.5)*2.0*str; c+=tex.Sample(samp,uv+dir*t*ts*5.0).rgb; } return c/16.0; }
float3 fx_tiltShift(Texture2D tex,SamplerState samp,float2 uv,float2 ts,float y0,float y1,float r){ float w=clamp((abs(uv.y-y0)-y1)/r,0,1); float3 c=float3(0,0,0); for(int i=-4;i<=4;i++)for(int j=-4;j<=4;j++)c+=tex.Sample(samp,uv+float2(i,j)*ts*w).rgb; return c/81.0; }
float3 fx_crosshatch(Texture2D tex,SamplerState samp,float2 uv,float2 ts){ float3 c=tex.Sample(samp,uv).rgb; float l=fx_luminance(c); float h1=step(0.25,frac(uv.x*300+uv.y*300)); float h2=step(0.25,frac(uv.x*300-uv.y*300)); float hatch=1.0; if(l<0.25)hatch=h1*h2; else if(l<0.5)hatch=max(h1,h2); else if(l<0.75)hatch=min(h1,h2); return c*hatch; }
float3 fx_emboss(Texture2D tex,SamplerState samp,float2 uv,float2 ts,float strength){ float3 tl=tex.Sample(samp,uv+float2(-1,-1)*ts).rgb; float3 br=tex.Sample(samp,uv+float2(1,1)*ts).rgb; return(tl-br)*strength+0.5; }
float fx_edgeDetect(Texture2D tex,SamplerState samp,float2 uv,float2 ts){ float tl=fx_luminance(tex.Sample(samp,uv+float2(-1,-1)*ts).rgb); float tr=fx_luminance(tex.Sample(samp,uv+float2(1,-1)*ts).rgb); float bl=fx_luminance(tex.Sample(samp,uv+float2(-1,1)*ts).rgb); float br=fx_luminance(tex.Sample(samp,uv+float2(1,1)*ts).rgb); return length(float2((tr+br)-(tl+bl),(bl+br)-(tl+tr))); }

// ==== 调试/可视化 ============================================================
float3 fx_falseColor(float t){ return fx_sat(float3(smoothstep(0.5,1.0,t),1.0-abs(t*2.0-1.0),smoothstep(0.5,0.0,t))); }
float3 fx_showLuminance(float3 c){ return fx_falseColor(fx_luminance(c)); }
float3 fx_showOverexposed(float3 c,float t){ return lerp(c,float3(1,0,0),step(t,fx_luminance(c))); }
float3 fx_showUnderexposed(float3 c,float t){ return lerp(c,float3(0,0,1),1.0-step(t,fx_luminance(c))); }

// ==== library inlined ====

struct FSInput {
    [[vk::location(0)]] float2 uv    : TEXCOORD0;
    [[vk::location(1)]] float4 color : COLOR0;
    [[vk::location(2)]] float4 props : TEXCOORD2;
    float4 position : SV_Position;
};
[[vk::binding(1, 0)]] SamplerState imageSampler;
[[vk::binding(2, 0)]] Texture2D<float4> imageTex;

[shader("fragment")]
float4 fragmentMainD(FSInput input) : SV_Target
{
    float4 texColor = imageTex.Sample(imageSampler, input.uv);
    float3 color = texColor.rgb;
    // 链 D: 冷暖色混合 + ToneMapping 对比
    float3 warm = fx_colorTemp(color, 0.6);
    float3 cool = fx_colorTemp(color, -0.4);
    color = fx_blendSoftLight(warm, cool);
    color = lerp(color, fx_sepia(color), 0.15);
    color = fx_lottes(color);
    color = fx_liftGammaGain(color, float3(-0.05,-0.03,-0.02), float3(1.1,1.05,1.0), float3(1.05,1.0,0.95));
    color = fx_crtEffect(color, input.uv, 240.0);
    color = fx_saturation(color, 1.15);
    return float4(fx_sat(color), texColor.a);
}
