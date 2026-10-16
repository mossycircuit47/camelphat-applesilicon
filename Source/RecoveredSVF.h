#pragma once
#include "RecoveredFilters.h"
#include <array>
namespace camelphat_recovered {
// Direct scalar translation of SVFAFilter 0x55680..0x560a0.
struct SVFAFilter {
 float sampleRate=88200,invRate=0,cutoff=5,targetCutoff=5,resonance=0,targetResonance=0,drive=1,targetDrive=1;
 std::array<float,4> band{},low{};std::uint32_t seed=0;int mode=0;bool dirty=true;
 float f=0,k=0,inputGain=0,outputGain=0,df=0,dk=0,di=0,dout=0,targetInput=0,targetOutput=0;
 void setSampleRate(float v){sampleRate=v;invRate=1.0f/v;dirty=true;}
 void setDestCutoff(float midi){targetCutoff=midi/12.0f;dirty=true;}
 void setDestResonance(float v){targetResonance=v*floatBits(0x3f7d70a4);dirty=true;}
 void setDestDrive(float v){v=v-1.0f;targetDrive=1.0f+v;dirty=true;}
 void setMode(int v){mode=v%3;}
 void setPhatMode(unsigned){}
 void clearHistory(){cutoff=targetCutoff;resonance=targetResonance;drive=targetDrive;band.fill(0);low.fill(0);}
 static float sat(float x){if(x< -2.0f)return -1.0f;if(x>2.0f)return 1.0f;float y=x*x;y=y*x;float t=std::fabs(x)*0.0625f;t=t-0.25f;y=y*t;return y+x;}
 static float vectorSat(float x){x=std::min(2.f,std::max(-2.f,x));float a=std::fabs(x)*0.0625f;a=a+(-0.25f);float b=x*x;b=b*x;b=b*a;return x+b;}
 float frequency(float c){float x=std::pow(2.f,c-5.75f);x=x*440.f;x=x*invRate;double d=double(x)*doubleBits(0x400921fb54442d18ULL);d=d*0.5;d=d*doubleBits(0x400921fb54442d18ULL);d=d*0.5;float a=float(d);a=std::min(a,floatBits(0x3f490fdb));double z=std::sin(double(a));z=z+z;return float(z);}
 void recalc(int n){float invN=1.f/float(n);f=frequency(cutoff);float tf=frequency(targetCutoff);df=(tf-f)*invN;k=resonance*floatBits(0xc00147ae);k=k+2.f;k=std::max(k,floatBits(0xbd23d70a));k=std::min(k,2.f);float tk=targetResonance*floatBits(0xc00147ae);tk=tk+2.f;tk=std::max(tk,floatBits(0xbd23d70a));tk=std::min(tk,2.f);dk=(tk-k)*invN;inputGain=drive*0.25f;targetInput=targetDrive*0.25f;float d=drive-1.f;d=d*floatBits(0x3e4ccccd);d=d+1.f;outputGain=4.f/d;d=targetDrive-1.f;d=d*floatBits(0x3e4ccccd);d=d+1.f;targetOutput=4.f/d;di=(targetInput-inputGain)*invN;dout=(targetOutput-outputGain)*invN;dirty=false;}
 void process(float* data,int n){if(dirty)recalc(n);float b=band[0],l=low[0],ig=inputGain,og=outputGain;
  for(int i=0;i<n;++i){float correction=floatBits(0x3e2ab368)*k;correction=correction*f;correction=correction*f;float gain=f-correction;seed=seed*0x0bb38435u+0x3619636bu;float s=sat(b);s=s*gain;l=l+s;float damping=k-correction;damping=2.f-damping;damping=damping*b;float damped=sat(damping);float h=data[i]*ig;float noise=float(seed)*floatBits(0x27a7c5ac);h=h+noise;h=h-l;float t=b*(-2.f);h=h+t;h=h+damped;float step=gain*sat(h);b=b+step;if(mode==0)data[i]=l*og;else if(mode==1)data[i]=b*og;else if(mode==2)data[i]=h*og;f=f+df;k=k+dk;if(i+1<n){ig=ig+di;og=og+dout;}}
  band[0]=b;low[0]=l;cutoff=targetCutoff;resonance=targetResonance;drive=targetDrive;inputGain=targetInput;outputGain=targetOutput;df=dk=di=dout=0;
 }
 void process2(float* left,float* right,int n){if(dirty)recalc(n);float ig=inputGain,og=outputGain;
  for(int i=0;i<n;++i){float sq=f*f;float correction=floatBits(0x3e2ab368)*k;correction=correction*sq;float gain=f-correction;float damping=k-correction;seed=seed*0x0bb38435u+0x3619636bu;float noise=float(seed)*floatBits(0x27a7c5ac);
   for(int ch=0;ch<4;++ch){float h=(ch==0?left[i]:(ch==1?right[i]:0.f))*ig;h=h+noise;float s=vectorSat(band[ch]);s=s*gain;low[ch]=low[ch]+s;float t=2.f-damping;t=t*band[ch];t=vectorSat(t);float delta=band[ch]*2.f;delta=delta-t;h=h-low[ch];h=h-delta;t=vectorSat(h);t=gain*t;band[ch]=band[ch]+t;float out=mode==0?low[ch]*og:(mode==1?band[ch]*og:h*og);if(ch==0)left[i]=out;else if(ch==1)right[i]=out;}
   f=f+df;k=k+dk;ig=ig+di;og=og+dout;
  }cutoff=targetCutoff;resonance=targetResonance;drive=targetDrive;inputGain=targetInput;outputGain=targetOutput;df=dk=di=dout=0;
 }
};

struct OTAAFilter {
 float sampleRate=88200,cutoff=5,targetCutoff=5,resonance=0,targetResonance=0,drive=1,targetDrive=1;
 std::array<float,4> stages{},previous{};std::array<std::array<float,4>,4> stereoStages{},stereoPrevious{};std::array<float,4> stereoLast{};float last=0,g=0,feedback=0,dg=0,dfb=0,inputGain=0,outputGain=0,targetInput=0,targetOutput=0,di=0,dout=0;
 std::uint32_t seed=0;int mode=0;bool dirty=true;
 void setSampleRate(float v){sampleRate=v;dirty=true;}
 void setDestCutoff(float v){targetCutoff=v/12.f;dirty=true;}
 void setDestResonance(float v){targetResonance=v*floatBits(0x3f7d70a4);dirty=true;}
 void setDestDrive(float v){targetDrive=v;dirty=true;}
 void setMode(int v){mode=v;}
 void setPhatMode(unsigned){}
 void clearHistory(){cutoff=targetCutoff;resonance=targetResonance;drive=targetDrive;stages.fill(0);previous.fill(0);for(auto& a:stereoStages)a.fill(0);for(auto& a:stereoPrevious)a.fill(0);stereoLast.fill(0);last=0;dirty=true;}
 void coefficients(float c,float r,float& cg,float& cf){float hz=std::pow(2.f,c-5.75f);hz=hz*440.f;float cap=sampleRate*floatBits(0x3e8a3d71);float x=std::min(cap,hz);x=x+x;x=x/sampleRate;double z=x,z2=z*z,z4=z2*z2,z6=z4*z2;
 double t=z*doubleBits(0x3feff209f7dc5654ULL);t=t+z2*doubleBits(0xbffa379386d5809bULL);double q=z2*doubleBits(0x40088dcf8fe2dc7dULL);q=q*z;t=t+q;t=t+z4*doubleBits(0xc0134e8834f06b06ULL);q=z4*doubleBits(0x4013b7b4f143a6b7ULL);q=q*z;t=t+q;t=t+z6*doubleBits(0xc0073267e8041650ULL);q=z6*doubleBits(0x3fe7862638de9408ULL);q=q*z;t=t+q;t=t*doubleBits(0x400921fb54442d18ULL);cg=float(t);
 t=z*doubleBits(0x3fccfd0ccd2859a3ULL);t=t+doubleBits(0x3ff001b8334572f2ULL);t=t+z2*doubleBits(0xbff46762780da04aULL);q=doubleBits(0x3ff6cb824d633f6aULL)*z2;q=q*z;t=t+q;q=z2*z2;q=q*doubleBits(0xbfe0230d9b40a852ULL);t=t+q;cf=float(t);cf=cf*0.625f;float rr=4.f*r;cf=cf*rr;
 }
 void recalc(int n){float invN=1.f/float(n);float tg,tf;coefficients(cutoff,resonance,g,feedback);coefficients(targetCutoff,targetResonance,tg,tf);dg=(tg-g)*invN;dfb=(tf-feedback)*invN;inputGain=drive*0.25f;targetInput=targetDrive*0.25f;float t=drive-1.f;t=t*floatBits(0x3e4ccccd);t=t+1.f;outputGain=4.f/t;t=targetDrive-1.f;t=t*floatBits(0x3e4ccccd);t=t+1.f;targetOutput=4.f/t;di=(targetInput-inputGain)*invN;dout=(targetOutput-outputGain)*invN;dirty=dg!=0.f||dfb!=0.f;}
 float output(float x){static constexpr float table[6][5]={{0,0,0,0,1},{0,0,3,-6,3},{1,-4,6,-4,1},{0,0,1,0,0},{0,1.5f,-1.5f,0,0},{1,-2,1,0,0}};if(mode<0||mode>5)return x;auto& t=table[mode];float o=0;if(mode==0)o=t[4]*stages[3];else if(mode==3)o=t[2]*stages[1];else if(mode==4){o=t[1]*stages[0];o=o+t[2]*stages[1];}else if(mode==5){o=t[0]*x;o=o+t[1]*stages[0];o=o+t[2]*stages[1];}else if(mode==1){o=t[2]*stages[1];o=o+t[3]*stages[2];o=o+t[4]*stages[3];}else {o=t[0]*x;o=o+t[1]*stages[0];o=o+t[2]*stages[1];o=o+t[3]*stages[2];o=o+t[4]*stages[3];}return o*outputGain;}
 void process(float* data,int n){if(dirty)recalc(n);float ig=inputGain,og=outputGain;for(int i=0;i<n;++i){float x=ig*data[i];float t=4.f*x;t=t*(-0.5f);t=t+last;t=t*floatBits(0x3fcccccd);t=SVFAFilter::sat(t);t=t*feedback;x=x-t;std::uint32_t r1=seed*0x0bb38435u+0x3619636bu;seed=r1*0x0bb38435u+0x3619636bu;
 for(int stage=0;stage<4;++stage){float input=stage==0?x:stages[static_cast<size_t>(stage-1)];float t=floatBits(0x3f44ec4f)*input;previous[static_cast<size_t>(stage)]=previous[static_cast<size_t>(stage)]*floatBits(0x3e6c4ec5);t=t+previous[static_cast<size_t>(stage)];t=t-stages[static_cast<size_t>(stage)];if(stage==0||stage==2){float noise=float(static_cast<std::int32_t>(stage==0?r1:seed))*floatBits(0x23d6bf95);t=t+noise;}t=SVFAFilter::sat(t);t=t*g;stages[static_cast<size_t>(stage)]=stages[static_cast<size_t>(stage)]+t;}
 float saved=outputGain;outputGain=og;data[i]=output(x);outputGain=saved;g=g+dg;feedback=feedback+dfb;if(i+1<n){ig=ig+di;og=og+dout;}previous[0]=x;previous[1]=stages[0];previous[2]=stages[1];previous[3]=stages[2];last=stages[3];}
 cutoff=targetCutoff;resonance=targetResonance;drive=targetDrive;inputGain=targetInput;outputGain=targetOutput;dg=dfb=di=dout=0;
 }
 void process2(float* left,float* right,int n){if(dirty)recalc(n);float ig=inputGain,og=outputGain;for(int i=0;i<n;++i){seed=seed*0x0bb38435u+0x3619636bu;float noise=float(static_cast<std::int32_t>(seed))*floatBits(0x23d6bf95);
  for(int ch=0;ch<4;++ch){auto& st=stereoStages[static_cast<size_t>(ch)];auto& pr=stereoPrevious[static_cast<size_t>(ch)];float x=(ch==0?left[i]:(ch==1?right[i]:0.f))*ig;float t=x*2.f;stereoLast[static_cast<size_t>(ch)]=stereoLast[static_cast<size_t>(ch)]-t;t=stereoLast[static_cast<size_t>(ch)]*floatBits(0x3fcccccd);t=SVFAFilter::vectorSat(t);t=t*feedback;x=x-t;
   for(int stage=0;stage<4;++stage){float in=stage==0?x:st[static_cast<size_t>(stage-1)];float v=floatBits(0x3f44ec4f)*in;float prev=pr[static_cast<size_t>(stage)]*floatBits(0x3e6c4ec5);v=v+prev;if(stage==0||stage==2){float q=noise-st[static_cast<size_t>(stage)];v=v+q;}else v=v-st[static_cast<size_t>(stage)];v=SVFAFilter::vectorSat(v);v=v*g;st[static_cast<size_t>(stage)]=st[static_cast<size_t>(stage)]+v;pr[static_cast<size_t>(stage)]=in;}
   stereoLast[static_cast<size_t>(ch)]=st[3];auto savedStages=stages;float savedGain=outputGain;stages=st;outputGain=og;float out=output(x);stages=savedStages;outputGain=savedGain;if(ch==0)left[i]=out;else if(ch==1)right[i]=out;
  }g=g+dg;feedback=feedback+dfb;ig=ig+di;og=og+dout;
 }cutoff=targetCutoff;resonance=targetResonance;drive=targetDrive;inputGain=ig;outputGain=og;dg=dfb=di=dout=0;
 }

};
}
