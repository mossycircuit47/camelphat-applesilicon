#pragma once
#include "RecoveredLookupTables.h"
#include "RecoveredFilters.h"
#include <array>
namespace camelphat_recovered {
// MS20AFilter 0x52450 / 0x52A10 / 0x53110.
struct MS20AFilter {
    float sampleRate=88200,cutoff=5,destCutoff=5,resonance=0,destResonance=0,drive=1,destDrive=1;
    float a[2]{},b[2]{},frequency=0,destFrequency=0,g=0,destG=0,gStep=0,k=0,destK=0,kStep=0,pre=0,destPre=0,preStep=0,post=0,destPost=0,postStep=0;
    int mode=0;uint32_t random=0;bool dirty=true;
    void setSampleRate(float sr){sampleRate=sr;dirty=true;}
    void setDestCutoff(float note){destCutoff=note/12.0f;dirty=true;}
    void setDestResonance(float r){destResonance=r*floatBits(0x3F7D70A4);dirty=true;}
    void setDestDrive(float d){destDrive=d;dirty=true;}
    void setMode(int m){mode=m;}
    void clearHistory(){cutoff=destCutoff;resonance=destResonance;drive=destDrive;a[0]=a[1]=b[0]=b[1]=0;g=destG=k=destK=0;dirty=true;}
    static double diode(double input){
        const double absolute=std::fabs(input),scaled=absolute/doubleBits(0x40B25C0000000000ULL);double x=input*1.5;
        for(int i=0;i<250;++i){double delta=x-absolute;
            double e=std::exp((0.5*delta)/doubleBits(0x3F9999999999999AULL));
            double deriv=doubleBits(0x3E55798EE2308C3AULL)*e+doubleBits(0x3F1A36E2EB1C432DULL);
            if(doubleBits(0x3EB0C6F7A0B5ED8DULL)>std::fabs(deriv))break;
            double change=delta/doubleBits(0x40C3880000000000ULL)-scaled;
            change+=(e-1.0)*doubleBits(0x3E112E0BE826D695ULL);change/=deriv;
            if(doubleBits(0x3EB0C6F7A0B5ED8DULL)>std::fabs(change))break;
            x-=change;
        }
        x=std::min(8.0,x);return input>=0?x:-x;
    }
    static const std::array<float,4100>& diodeTable(){return originalDiodeLut;}
    static float saturate(float x){x=std::max(-2.0f,std::min(2.0f,x));float f=std::fabs(x)*0.0625f-0.25f;float c=x*x;c*=x;c*=f;return x+c;}
    void recalc(int n){float scale=1.0f/float(n),inv=1.0f/sampleRate;
        frequency=(std::pow(2.0f,cutoff-5.75f)*440.0f)*inv;destFrequency=(440.0f*std::pow(2.0f,destCutoff-5.75f))*inv;
        auto makeG=[](float f){float angle=float(double(f)*doubleBits(0x400921FB54442D18ULL));return float(std::sin(double(std::min(angle,floatBits(0x3FA0D97C))))*2.0);};
        g=makeG(frequency);destG=makeG(destFrequency);gStep=(destG-g)*scale;
        auto makeK=[](float r,float coeff){float v=std::min(4.0f,std::max(0.0f,r+r));double q=(double(coeff)*1.5)*double(v);float t=v+v;t+=1.0f;float subtract=float(q-double(t));return v-std::max(0.0f,subtract);};
        k=makeK(resonance,g);destK=makeK(destResonance,destG);kStep=(destK-k)*scale;
        pre=drive*floatBits(0x3DCCCCCD);destPre=destDrive*floatBits(0x3DCCCCCD);preStep=(destPre-pre)*scale;
        auto makePost=[](float d){float x=d-1.0f;x*=floatBits(0x3E4CCCCD);x+=1.0f;return 10.0f/x;};
        post=makePost(drive);destPost=makePost(destDrive);postStep=(destPost-post)*scale;dirty=false;
    }
    float diodeLookup(float state) const {float index=k*state;index*=128.0f;index+=2048.0f;index=std::max(0.0f,std::min(floatBits(0x45801000),index));int i=int(index);const auto&t=diodeTable();return (t[size_t(i+1)]-t[size_t(i)])*(index-float(i))+t[size_t(i)];}
    void finish(){cutoff=destCutoff;resonance=destResonance;drive=destDrive;pre=destPre;post=destPost;gStep=kStep=preStep=postStep=0;}
    void setPhatMode(unsigned) {}
    void process(float* samples,int n){if(dirty)recalc(n);
        for(int i=0;i<n;++i){random=random*0x0BB38435u+0x3619636Bu;float input=pre*samples[i];input+=float(uint64_t(random))*floatBits(0x27A7C5AC);
            float oldA=a[0],oldB=b[0],d=diodeLookup(b[0]);
            float first=mode==2?-d:-input;if(mode!=2)first-=d;first-=a[0];
            a[0]+=saturate(first)*g;
            float second=0.5f*a[0]+oldA*0.5f;
            if(mode==2)second-=input;else if(mode==1)second+=input;
            second+=d;second-=b[0];b[0]+=saturate(second)*g;
            float output=0.5f*b[0];if(mode==2)output+=input;output+=oldB*0.5f;output*=post;samples[i]=output;
            g+=gStep;k+=kStep;pre+=preStep;post+=postStep;
        }finish();
    }
    void process1(float*l,float*,int n,void* = nullptr){process(l,n);}
    void process2(float*l,float*r,int n,void* = nullptr){if(dirty)recalc(n);
        for(int i=0;i<n;++i){random=random*0x0BB38435u+0x3619636Bu;float noise=float(uint64_t(random))*floatBits(0x27A7C5AC);
            for(int ch=0;ch<2;++ch){float input=pre*(ch?r[i]:l[i]);input+=noise;float oldA=a[ch],oldB=b[ch],d=diodeLookup(b[ch]);
                float first=mode==2?0.0f-(d+a[ch]):(0.0f-input)-(d+a[ch]);
                a[ch]+=saturate(first)*g;float second;
                if(mode==0){float q=d-b[ch];second=(0.5f*a[ch]+oldA*0.5f)+q;}
                else if(mode==1){float q=input+d;q-=b[ch];second=(0.5f*a[ch]+oldA*0.5f)+q;}
                else {float q=d-input;q-=b[ch];second=(0.5f*a[ch]+oldA*0.5f)+q;}
                b[ch]+=saturate(second)*g;
                float output=0.5f*b[ch];output+=oldB*0.5f;if(mode==2)output=input+output;output*=post;if(ch)r[i]=output;else l[i]=output;
            }g+=gStep;k+=kStep;pre+=preStep;post+=postStep;
        }finish();
    }
};
}
namespace camelphat_recovered {
struct MoogAFilter {
    float sampleRate=88200,cutoff=5,destCutoff=5,resonance=0,destResonance=0,drive=1,destDrive=1;
    float g=0,destG=0,gStep=0,k=0,destK=0,kStep=0,pre=0,destPre=0,preStep=0,post=0,destPost=0,postStep=0;
    float states[2][4]{},histories[2][4]{},monoStates[4]{},monoHistories[4]{},monoFeedback=0;
    int mode=0;uint32_t random=0;bool dirty=true;
    void setSampleRate(float sr){sampleRate=sr;dirty=true;}
    void setDestCutoff(float note){destCutoff=note/12.0f;dirty=true;}
    void setDestResonance(float r){destResonance=r*floatBits(0x3F7D70A4);dirty=true;}
    void setDestDrive(float d){destDrive=d;dirty=true;}
    void setMode(int m){mode=m;dirty=true;}
    void setPhatMode(unsigned){}
    void clearHistory(){cutoff=destCutoff;resonance=destResonance;drive=destDrive;for(auto&s:states)for(auto&x:s)x=0;for(auto&s:histories)for(auto&x:s)x=0;for(auto&x:monoStates)x=0;for(auto&x:monoHistories)x=0;monoFeedback=0;dirty=true;}
    static void coefficients(float normalized,float resonance,float& g,float& feedback){
        double x=double(normalized),x2=x*x,x4=x2*x2,x6=x4*x2;
        double t=doubleBits(0x3FEFF209F7DC5654ULL)*x;
        t+=x2*doubleBits(0xBFFA379386D5809BULL);t+=(x2*doubleBits(0x40088DCF8FE2DC7DULL))*x;
        t+=x4*doubleBits(0xC0134E8834F06B06ULL);t+=(x4*doubleBits(0x4013B7B4F143A6B7ULL))*x;
        t+=x6*doubleBits(0xC0073267E8041650ULL);t+=(x6*doubleBits(0x3FE7862638DE9408ULL))*x;
        g=float(t*doubleBits(0x400921FB54442D18ULL));
        t=x*doubleBits(0x3FCCFD0CCD2859A3ULL)+doubleBits(0x3FF001B8334572F2ULL);
        t+=x2*doubleBits(0xBFF46762780DA04AULL);t+=(x2*doubleBits(0x3FF6CB824D633F6AULL))*x;
        t+=x4*doubleBits(0xBFE0230D9B40A852ULL);feedback=float(t)*resonance;
    }
    void recalc(int n){float scale=1.0f/float(n),cap=floatBits(0x3E8A3D71)*sampleRate;
        float hz=std::pow(2.0f,cutoff-5.75f)*440.0f;float destHz=440.0f*std::pow(2.0f,destCutoff-5.75f);
        coefficients(float((2.0*double(std::min(hz,cap)))/double(sampleRate)),resonance*4.0f,g,k);
        coefficients(float((2.0*double(std::min(destHz,cap)))/double(sampleRate)),destResonance*4.0f,destG,destK);
        gStep=(destG-g)*scale;kStep=(destK-k)*scale;
        float multiplier=(mode==0||mode==3)?1.0f:0.125f;
        pre=drive*multiplier;destPre=destDrive*multiplier;
        auto makePost=[this](float d){float t=d-1.0f;t*=floatBits(0x3E4CCCCD);t+=1.0f;return ((mode==0||mode==3)?1.0f:8.0f)/t;};
        post=makePost(drive);destPost=makePost(destDrive);preStep=(destPre-pre)*scale;postStep=(destPost-post)*scale;
        dirty=gStep!=0.0f||kStep!=0.0f;
    }
    static float tanhCore(float x){return MS20AFilter::saturate(x);}
    float mix(float input,const float*s)const{
        switch(mode){case 0:return s[3];case 1:{float out=3.0f*s[1];out+=-6.0f*s[2];out+=3.0f*s[3];return out;}
        case 2:{float out=input;out+=-4.0f*s[0];out+=6.0f*s[1];out+=-4.0f*s[2];out+=s[3];return out;}
        case 3:return s[1];case 4:return 1.5f*s[0]+-1.5f*s[1];case 5:{float out=input;out+=-2.0f*s[0];out+=s[1];return out;}default:return input;}
    }
    float noise(){random=random*0x0BB38435u+0x3619636Bu;return float(int32_t(random))*floatBits(0x23D6BF95);}
    void finish(){cutoff=destCutoff;resonance=destResonance;drive=destDrive;pre=destPre;post=destPost;gStep=kStep=preStep=postStep=0;}
    void process2(float*l,float*r,int n,void* = nullptr){if(dirty)recalc(n);
        float saturated[2][4];for(int c=0;c<2;++c)for(int s=0;s<4;++s)saturated[c][s]=tanhCore(states[c][s]);
        for(int i=0;i<n;++i){float noise1=noise(),noise2=noise();
            for(int c=0;c<2;++c){float input=(c?r[i]:l[i])*pre;input-=states[c][3]*k;float newInput=tanhCore(input);
                for(int s=0;s<4;++s){float current=s==0?newInput:saturated[c][s-1];float step;
                    if(s==0||s==2){float q=(s==0?noise1:noise2)-saturated[c][s];float old=histories[c][s]*floatBits(0x3E6C4EC5);step=floatBits(0x3F44EC4F)*current;step+=old;step+=q;}
                    else {float old=histories[c][s]*floatBits(0x3E6C4EC5);step=floatBits(0x3F44EC4F)*current;step+=old;step-=saturated[c][s];}
                    step*=g;states[c][s]+=step;histories[c][s]=current;saturated[c][s]=tanhCore(states[c][s]);
                }
                float out=mix(input,states[c])*post;if(c)r[i]=out;else l[i]=out;
            }g+=gStep;k+=kStep;pre+=preStep;post+=postStep;
        }finish();
    }
    void process(float*samples,int n){if(dirty)recalc(n);float sat[4];for(int s=0;s<4;++s)sat[s]=tanhCore(monoStates[s]);
        for(int i=0;i<n;++i){float input=pre*samples[i]-monoFeedback*k;float current=tanhCore(input),noise1=noise(),noise2=noise();
            for(int s=0;s<4;++s){float step=monoHistories[s]*floatBits(0x3E6C4EC5);step+=current*floatBits(0x3F44EC4F);step-=sat[s];if(s==0)step+=noise1;if(s==2)step+=noise2;step*=g;monoStates[s]+=step;monoHistories[s]=current;current=tanhCore(monoStates[s]);sat[s]=current;}
            g+=gStep;k+=kStep;post+=postStep;samples[i]=mix(input,monoStates)*post;pre+=preStep;monoFeedback=monoStates[3];
        }finish();
    }
    void process1(float*l,float*,int n,void* = nullptr){process(l,n);}
};
}
