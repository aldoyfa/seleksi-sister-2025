#include <stdio.h>

#define MAXD 1000005
#define MAXN (1<<21) /* 2,097,152 >= 2*1,000,000 */
#define MOD 998244353
#define G 3

static char num1[MAXD];
static char num2[MAXD];
static int a_ntt[MAXN];
static int b_ntt[MAXN];
static int rev_bits[MAXN];
static int digits[2200005]; /* result digits after carry */

/* Bitwise add */
int add(int x, int y){ if(!y) return x; return add(x ^ y, (x & y) << 1); }
/* Bitwise subtract (x - y) */
int sub(int x, int y){ if(!y) return x; return sub(x ^ y, (~x & y) << 1); }
/* a<b ? (signed) */
int less_than(int a, int b){ int d = sub(a,b); return (d>>31) & 1; }

/* modular add */
int addmod(int x,int y){ int s=add(x,y); if(!less_than(s,MOD)) s=sub(s,MOD); return s; }
/* modular sub */
int submod(int x,int y){ int s=sub(x,y); if(less_than(s,0)) s=add(s,MOD); return s; }

/* multiply modulo MOD (double-and-add) */
int mul_mod(int x,int y){
    int res=0;
    mul_loop:
    if(!y) goto mul_end;
    if(y & 1) res=addmod(res,x);
    x=addmod(x,x);
    y = y >> 1;
    goto mul_loop;
    mul_end:
    return res;
}

/* fast power modulo MOD */
int pow_mod(int a,int e){
    int r=1;
    pow_loop:
    if(!e) goto pow_end;
    if(e & 1) r=mul_mod(r,a);
    a=mul_mod(a,a);
    e = e >> 1;
    goto pow_loop;
    pow_end:
    return r;
}

int get_length(char *s){ int l=0; gl_loop: if(!(s[l])) goto gl_end; l=add(l,1); goto gl_loop; gl_end: return l; }

/* Precompute bit-reversed indices */
void build_rev(int n,int logn){
    int i=0; br_loop:
    if(!less_than(i,n)) goto br_end;
    int x=i; int r=0; int k=0;
    br_bit_loop:
    if(!less_than(k,logn)) goto br_bit_end;
    r = r << 1; /* shift */
    if(x & 1) r = add(r,1);
    x = x >> 1;
    k = add(k,1);
    goto br_bit_loop;
    br_bit_end:
    rev_bits[i]=r;
    i=add(i,1);
    goto br_loop;
    br_end: ;
}

void ntt(int *a,int n,int invert){
    int logn=0; int t=n; while(t>1){ logn=add(logn,1); t = t >> 1; }
    build_rev(n,logn);
    int i=0; nr_loop:
    if(!less_than(i,n)) goto nr_end;
    if(less_than(i, rev_bits[i])){ int tmp=a[i]; a[i]=a[rev_bits[i]]; a[rev_bits[i]]=tmp; }
    i=add(i,1); goto nr_loop; nr_end: ;
    int root = G;
    int root_inv = pow_mod(root, sub(MOD,2));
    int len=2; int shift=1; len_loop:
    if(!less_than(len, add(n,1))) goto len_end; /* while len <= n */
    /* exponent = (MOD-1) / len since len is power of two -> right shift */
    int exponent_shifted = (MOD-1) >> shift;
    int wlen = pow_mod(invert ? root_inv : root, exponent_shifted);
    int i2=0; outer_loop:
    if(!less_than(i2,n)) goto outer_end;
    int w=1; int j=0; inner_loop:
    if(!less_than(j, (len>>1))) goto inner_end;
    int u=a[add(i2,j)];
    int v=mul_mod(a[add(add(i2,j),(len>>1))], w);
    a[add(i2,j)] = addmod(u,v);
    a[add(add(i2,j),(len>>1))] = submod(u,v);
    w = mul_mod(w, wlen);
    j = add(j,1); goto inner_loop; inner_end: ;
    i2 = add(i2,len); goto outer_loop; outer_end: ;
    len = len << 1; shift=add(shift,1); goto len_loop; len_end: ;
    if(invert){
        int n_inv = pow_mod(n, sub(MOD,2));
        int k=0; inv_loop:
        if(!less_than(k,n)) goto inv_end;
        a[k]=mul_mod(a[k], n_inv);
        k=add(k,1); goto inv_loop; inv_end: ;
    }
}

/* divide x by 10: outputs quotient in *q and remainder in *r using shift-subtract */
void divmod10(int x,int *q,int *r){
    int tmp=10; int mult=1; /* find highest double */
    /* scale */
    dm_scale:
    if(less_than(x, tmp<<1)) goto dm_scale_end; /* while (tmp<<1) <= x */
    tmp = tmp << 1; mult = mult << 1; goto dm_scale;
    dm_scale_end: ;
    int qq=0;
    dm_red:
    if(!mult) goto dm_done;
    if(!less_than(x,tmp)){ x=sub(x,tmp); qq=add(qq,mult); }
    tmp = tmp >> 1; mult = mult >> 1; goto dm_red;
    dm_done:
    *q = qq; *r = x;
}

void big_multiply(char *s1,char *s2,int len1,int len2){
    int n1=len1; int n2=len2;
    int n=1; int need=add(n1,n2); need=sub(need,1);
    grow_loop:
    if(!less_than(n, need)) goto grow_end; /* while n < need */
    n = n << 1; goto grow_loop;
    grow_end: ;
    /* Clear arrays */
    int i=0; cl_loop:
    if(!less_than(i,n)) goto cl_end;
    a_ntt[i]=0; b_ntt[i]=0; i=add(i,1); goto cl_loop; cl_end: ;
    /* load digits reversed */
    i=0; ld1:
    if(!less_than(i,n1)) goto ld1_end;
    a_ntt[i]=sub(s1[sub(sub(n1,1),i)], '0');
    i=add(i,1); goto ld1; ld1_end: ;
    i=0; ld2:
    if(!less_than(i,n2)) goto ld2_end;
    b_ntt[i]=sub(s2[sub(sub(n2,1),i)], '0');
    i=add(i,1); goto ld2; ld2_end: ;
    ntt(a_ntt,n,0); ntt(b_ntt,n,0);
    i=0; pm_loop:
    if(!less_than(i,n)) goto pm_end;
    a_ntt[i]=mul_mod(a_ntt[i], b_ntt[i]);
    i=add(i,1); goto pm_loop; pm_end: ;
    ntt(a_ntt,n,1);
    /* carry over base 10 */
    int carry=0; int q=0; int r=0; int res_len=add(n1,n2); /* upper bound */
    i=0; car_loop:
    if(!less_than(i,res_len)) goto car_end;
    int val = add(a_ntt[i], carry);
    divmod10(val, &carry, &r); /* carry=val/10, r=val%10 */
    digits[i]=r;
    i=add(i,1); goto car_loop; car_end: ;
    /* consume remaining carry */
    cc_loop:
    if(less_than(carry,10)) goto cc_last;
    divmod10(carry,&carry,&r); digits[res_len]=r; res_len=add(res_len,1); goto cc_loop;
    cc_last: if(carry){ digits[res_len]=carry; res_len=add(res_len,1);} ;
    /* trim leading zeros */
    trim_loop:
    if(!less_than(1,res_len)) goto trim_end;
    if(digits[sub(res_len,1)]) goto trim_end;
    res_len=sub(res_len,1); goto trim_loop; trim_end: ;

    i=sub(res_len,1);
    out_loop:
    if(less_than(i,0)) goto out_end;
    printf("%d", digits[i]);
    i=sub(i,1); goto out_loop; out_end: ;
}

int main(){
    if(scanf("%s %s", num1, num2)!=2){ return 0; }
    int len1=get_length(num1); int len2=get_length(num2);
    
    /* Check for zero*/
    int is_zero1 = !(sub(len1, 1)) & !(sub(num1[0], '0'));
    int is_zero2 = !(sub(len2, 1)) & !(sub(num2[0], '0'));
    
    if(is_zero1 | is_zero2){ printf("0\n"); return 0; }
    big_multiply(num1,num2,len1,len2);
    printf("\n");
    return 0;
}