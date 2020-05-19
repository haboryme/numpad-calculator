#include <avr_f64.h>

static char TemporaryMemory[128];


static void f_split64(float64_t *x, uint8_t * f_sign, int16_t * f_ex, uint64_t *frac, uint8_t lshift)
{	// Liefert *frac=0, wenn x die Zahl Null oder +/-INF darstellt.
	// Liefert *frac!=0, wenn x NaN ist.
	// Ansonsten wird in *frac auch das f�hrende 1-Bit der Mantisse erg�nzt, welches in *x nicht vorhanden ist (und implizit als 1 gilt).
	*frac = (*x) & 0xfffffffffffff;
	if(0==(*f_ex  =  (((*x)>>52) & 2047)))
		*frac=0;
	else if(2047!=(*f_ex))
		*frac |= 0x10000000000000;
	*frac <<= lshift;
	*f_sign = ((*x)>>63) & 1;
}


static void f_combi_from_fixpoint(float64_t * x, uint8_t f_sign, int16_t f_ex, uint64_t *frac)
{	// Achtung: NaN kann mit dieser Funktion nicht erzeugt werden.
	// Wenn *frac==0 �bergeben wird, wird Null erzeugt, falls f_ex<2047 ist, und sonst +/-INF.
	uint8_t round=0;
	uint64_t w=*frac;
	if(0!=w)
	{
		while(0==(w & 0xffffe00000000000))
		{
			w<<=8;
			f_ex-=8;
		}
		while(0==(w & 0xfff0000000000000))
		{
			w<<=1;
			--f_ex;
		}
		while(0!=(w & 0xff00000000000000))
		{
			round = 0!=(w&(1<<3));
			w>>=4;
			f_ex+=4;
		}
		while(0!=(w & 0xffe0000000000000))
		{
			round = 0!=(w&1);
			w>>=1;
			++f_ex;
		}
		if(round)
		{
			++w;
			if(0!=(w & 0xffe0000000000000))
			{
				w>>=1;
				++f_ex;
			}
		}
		if(f_ex<=0) // Falls f_ex==0: Es werden keine unnormalisierten Zahlen au�er Null unterst�tzt
			{ f_ex=0; w=0; } // +0 oder -0
	}
	else if(f_ex<2047)
		f_ex=0;
	if(f_ex>=2047)
		{ f_ex=2047; w=0; } // +INF oder -INF
	*((uint64_t*)x)=((uint64_t)f_sign<<63) | ((uint64_t)f_ex<<52) | (w & 0xfffffffffffff);
}

#if defined(F_WITH_fmod) || defined(F_WITH_exp) || defined(F_WITH_sin) || defined(F_WITH_cos) || defined(F_WITH_tan) || defined(F_WITH_to_decimalExp) || defined(F_WITH_to_string) || defined(F_WITH_strtod) || defined(F_WITH_atof)
static int8_t f_shift_left_until_bit63_set(uint64_t *w)
{	// Falls *w=0 ist oder falls das Bit mit der Nummer 63 nicht durch
	// mehrmaliges Linksschieben von *w gesetzt werden kann, wird *w=0 gesetzt und 64 zur�ckgeliefert.
	// Ansonsten wird *w so oft nach links geshiftet, bis das Bit mit der Nummer 63
	// gesetzt ist. Die Anzahl der erfolgten Links-Shifts wird zur�ckgeliefert.
	register int8_t count=0;
	register uint64_t mask;
	for(mask=((uint64_t)255LU)<<(63-7); 0==(mask & (*w)) && count<64; count+=8)
		(*w) <<= 8;
	for(mask=((uint64_t)1LU)<<63; 0==(mask & (*w)) && count<64; ++count)
		(*w) <<= 1;
	return count;
}
#endif

static uint64_t approx_high_uint64_word_of_uint64_mult_uint64(uint64_t *x, uint64_t *y, uint8_t signed_mult)
{	// Berechnet eine N�herung von floor(x * y / (2 hoch 64)). Wenn signed_mult==0 �bergeben wird, ist
	// der zur�ckgegebene Wert ist niemals gr��er und maximal um 3 kleiner als der tats�chliche Wert.
	// 0!=signed_mult & 1 : *x ist vorzeichenbehaftet
	// 0!=signed_mult & 2 : *y ist vorzeichenbehaftet
	uint64_t r=((*x)>>32)*((*y)>>32) + ((((*x)>>32)*((*y)&0xffffffff))>>32) + ((((*y)>>32)*((*x)&0xffffffff))>>32);
	if(0!=(signed_mult & 1) && ((int64_t)(*x))<0)
		r -= (*y);
	if(0!=(signed_mult & 2) && ((int64_t)(*y))<0)
		r -= (*x);
	return r;
}

static uint64_t approx_high_uint64_word_of_uint64_mult_uint64_pbv_y(uint64_t *x, uint64_t y, uint8_t signed_mult)
{
	return approx_high_uint64_word_of_uint64_mult_uint64(x, &y, signed_mult);
}

static uint64_t approx_inverse_of_fixpoint_uint64(uint64_t *y)
{	// Es muss 0!=*y & 0x8000000000000000 sein !
	// Unter dieser Voraussetzung wird eine N�herung f�r (2 hoch 126) / *y berechnet.
	// Theoretische �berlegungen ergeben, dass nahezu alle 64 Bits des Ergebnisses signifkant sind.
	// Numerische Experimente lassen vermuten, dass der zur�ckgegebene Wert um maximal +/- 1 gegen�ber dem
	// auf die n�chste ganze Zahl gerundeten Wert von (2 hoch 126) / *y abweicht.
	uint64_t yv=*y, z8, k5;
	uint16_t z3;
	uint32_t z5=(yv>>32), z6, k;
	uint8_t c;

	if(0!=(((unsigned long)yv)&0x80000000) && 0==++z5)
		z6=0;
	else
	{
		z3=(z5>>16);

		if(0==(z3&0x7fff))
		{
			if(0==(z5&0xffff))
				return (0xffffffffffffffff+1)-yv;
			z6=-((z5&0xffff)<<2);
		}
		else if(0xffff==z3)
			z6=-z5;
		else
		{
			z3=(uint16_t)(1 + (0xffffffff-(z3>>1)) / z3);
			z6=(((uint32_t)z3)<<17) - (z5 + ((z3*((0x20000+(uint32_t)z3)*(uint64_t)z5))>>32))-1;
		}
	}

	for(k=(uint32_t)(k5=(0x100000000 | ((uint64_t)z6))*z5 + (z5>>1)), c=k5>>32 ; 0!=c ; )
	{
		if(k+z5<k) ++c; // Nach einem 8-Bit-�berlauf von c wird die Schleife abgebrochen
		k+=z5;
		++z6;
	}

	z8=(((uint64_t)1LU)<<63) | ((uint64_t)z6)<<31;

	// In der folgenden Anweisung wird ein Integer-�berlauf bewusst ausgenutzt.
	// Es wird a=(2 hoch 64) -z -approx_high_uint64_word_of_uint64_mult_uint64_pbv_y(&z, yv<<1, 0) berechnet.
	k5 = (0xffffffffffffffff+1)-z8-approx_high_uint64_word_of_uint64_mult_uint64_pbv_y(&z8, yv<<1, 0); // Bei yv<<1 geht das h�chstwertigste Bit verloren
	z8=approx_high_uint64_word_of_uint64_mult_uint64(&z8, &k5, 0) + ((0==(k5&0x8000000000000000)) ? z8 : 0);
	return (z8+1)>>1;
}



/***********************************************************/
static void f_addsub2(float64_t * x, float64_t a, float64_t b,
               uint8_t flagadd, uint8_t * flagexd )
/***********************************************************/
{ // add positive doubles 
	uint8_t sig;
	int16_t aex,bex;
	uint64_t wa, wb;

	f_split64(&a,&sig,&aex,&wa, 10);
	f_split64(&b,&sig,&bex,&wb, 10);

	*flagexd=0;

#ifdef F_ONLY_NAN_NO_INFINITY
	if(2047==aex || 2047==bex)
	{
		*x=float64_ONE_POSSIBLE_NAN_REPRESENTATION;
		return;
	}
#else
	if(2047==aex) // a ist ein NaN oder +INF oder -INF
	{
		if(0==wa && (2047!=bex || (0==wb && flagadd)))
			*x=a;
		else
			*x=float64_ONE_POSSIBLE_NAN_REPRESENTATION;
		return;
	}
	else if(2047==bex)
	{
		*x=b;
		return;
	}
#endif

	if(!aex || aex+64<=bex)
		{*x=b; *flagexd=1; return;} // Alle denormalisierten Zahlen werden als Null interpretiert.
	if(!bex || bex+64<=aex)
		{*x=a; return;} // Alle denormalisierten Zahlen werden als Null interpretiert.

	if(flagadd)
	{
		if(aex>=bex)
			wa+=wb>>(aex-bex); // aex-bex<64 wurde schon durch die obige Abfrage if(!bex || bex+64<=aex) sichergestellt
		else
		{
			wa=wb+(wa>>(bex-aex)); // bex-aex<64 wurde schon durch die obige Abfrage if(!aex || aex+64<=bex) sichergestellt
			aex=bex;
		}
	}
	else
	{
		if(aex>bex || (aex==bex && wa>=wb))
			wa-=wb>>(aex-bex); // aex-bex<64 wurde schon durch die obige Abfrage if(!bex || bex+64<=aex) sichergestellt
		else
		{
			wa=wb-(wa>>(bex-aex)); // bex-aex<64 wurde schon durch die obige Abfrage if(!aex || aex+64<=bex) sichergestellt
			*flagexd=1;
			aex=bex;
		}
	}
	f_combi_from_fixpoint(x, 0, aex-10, &wa);
}

/***********************************************************/
static void f_setsign(float64_t * x, int8_t sign)
/***********************************************************/
{
	if(sign)
		*x |= 0x8000000000000000;
	else
		*x &= 0x7fffffffffffffff;
}

/***********************************************************/
static uint8_t f_getsign(float64_t x)
/***********************************************************/
{ uint64_t * px=&x;
 return ((uint8_t)(((*px)>>63)&1));
}

/***********************************************************/
float64_t f_add(float64_t a, float64_t b)
/***********************************************************/
{uint8_t signa,signb,signerg;
 uint8_t   flagexd;
 uint64_t  i64;
 float64_t * x = &i64;
 
 signa= f_getsign(a);
 signb= f_getsign(b);
 if(signa^signb)
 {  // diff. signs
   f_addsub2(x,a,b,0,&flagexd); // calc a-b
   signerg= ((flagexd^signa))&1;  
 }else{            // eq.   signs
   f_addsub2(x,a,b,1,&flagexd); // calc a+b
   signerg= signa;
 }
 f_setsign(x,signerg);

return(i64);
}

/***********************************************************/
float64_t f_sub(float64_t a, float64_t b)
/***********************************************************/
{uint8_t signb;
 float64_t  bloc=b;
 uint64_t  i64;
 
 signb= f_getsign(bloc);
 signb ^= 1;
 f_setsign(&bloc,signb);
 i64=f_add(a,bloc);
 return(i64);

}

/***********************************************************/
float64_t f_mult(float64_t fa, float64_t fb)
/***********************************************************/
{ //multiply doubles 
  uint8_t  asig,bsig;
  int16_t aex,bex;
  uint64_t am, bm;
  
  f_split64(&fa,&asig,&aex,&am, 11);
  f_split64(&fb,&bsig,&bex,&bm, 11);
  
#ifdef F_ONLY_NAN_NO_INFINITY
  if(2047==aex || 2047==bex)
		return float64_ONE_POSSIBLE_NAN_REPRESENTATION;
#else
  if(2047==aex) // a ist ein NaN oder +INF oder -INF
	{
		if(0!=am || (2047==bex && 0!=bm) || 0==bex)
			return float64_ONE_POSSIBLE_NAN_REPRESENTATION; // Auch +/-INF * Null ergibt NaN
		// (+/- INF) * (+/- INF) oder (+/- INF) * endliche Zahl au�er Null ergibt +/-INF
	}
	else if(2047==bex)
	{
		if(0!=bm || 0==aex)
			return float64_ONE_POSSIBLE_NAN_REPRESENTATION;
		aex=bex; am=bm;
	}
#endif
  else if(!aex || !bex)  // ge�ndert von Florian K�nigstein: Alle denormalisierten Zahlen werden als Null interpretiert.
  { // 0
      return float64_NUMBER_PLUS_ZERO;
  }else{
	  aex=aex+bex-(0x3ff+10);
	  am=approx_high_uint64_word_of_uint64_mult_uint64(&am, &bm, 0);
  }
  asig ^= bsig;
  f_combi_from_fixpoint(&fa, asig, aex, &am); // Hier darf von aex nichts subtrahiert werden, damit
	// auch aex==2047 (INF, NaN) nicht verf�lscht wird.
  return fa;
}

/***********************************************************/
float64_t f_div(float64_t x, float64_t y)
/***********************************************************/
{
	uint8_t  xsig, ysig;
	int16_t xex, yex;
	uint64_t xm, ym, i64;

	f_split64(&x,&xsig,&xex,&xm, 11);
	f_split64(&y,&ysig,&yex,&ym, 11);
#ifdef F_ONLY_NAN_NO_INFINITY
	if(2047==xex || 2047==yex || 0==yex)
		return float64_ONE_POSSIBLE_NAN_REPRESENTATION;
#else
	if(0==yex) { // divide by 0 wont work  . Ge�ndert von Florian K�nigstein: Alle denormalisierten Zahlen werden als Null interpretiert.
		return float64_ONE_POSSIBLE_NAN_REPRESENTATION;
	}
	else if(2047==xex) // x ist ein NaN oder +INF oder -INF
	{
		if(0!=xm || 2047==yex)
			return float64_ONE_POSSIBLE_NAN_REPRESENTATION;
		// Ansonsten: (+/- INF) / (endliche Zahl au�er Null)
		return (xsig^ysig) ? float64_MINUS_INFINITY : float64_PLUS_INFINITY;
	}
	else if(2047==yex) // x ist ein NaN oder +INF oder -INF
	{
		if(0==ym) // endliche Zahl / INF = Null
			return float64_NUMBER_PLUS_ZERO;
		return float64_ONE_POSSIBLE_NAN_REPRESENTATION;
	}
#endif
	else
	{
		i64=approx_high_uint64_word_of_uint64_mult_uint64_pbv_y(&xm, approx_inverse_of_fixpoint_uint64(&ym), 0);
		xex += 1023-yex;
	}
	f_combi_from_fixpoint(&x, xsig^ysig, xex-10, &i64);
	return x;
}

#if defined(F_WITH_to_decimalExp) || defined(F_WITH_to_string) || defined(F_WITH_strtod) || defined(F_WITH_atof)
static int16_t f_10HochN(int64_t n, uint64_t *res)
{
	uint64_t pot=((uint64_t)10)<<60;
	int16_t exp2=0, pot_exp2=3;
	uint8_t neg=0;
	*res=((uint64_t)1)<<63;
	if(n<0)
		{ neg=1; n=-n; }
	while(0 != n)
	{
		if(0 != (n & 1))
		{
			*res = approx_high_uint64_word_of_uint64_mult_uint64(res, &pot, 0);
			exp2+=pot_exp2+1-f_shift_left_until_bit63_set(res);
		}
		pot = approx_high_uint64_word_of_uint64_mult_uint64(&pot, &pot, 0);
		pot_exp2=(pot_exp2<<1)+1-f_shift_left_until_bit63_set(&pot);
		n >>= 1;
	}
	if(neg)
	{
		*res = approx_inverse_of_fixpoint_uint64(res);
		exp2=-exp2-f_shift_left_until_bit63_set(res);
	}
	return exp2;
}
#endif

#if defined(F_WITH_to_decimalExp) || defined(F_WITH_to_string)
char *f_to_decimalExp(float64_t x, uint8_t anz_dezimal_mantisse, uint8_t MantisseUndExponentGetrennt,
						int16_t *ExponentBasis10)
{	// f_to_decimalExp() converts the float64 to the decimal representation of the number x if x is
	// a real number or to the strings "+INF", "-INF", "NaN". If x is real, f_to_decimalExp() generates
	// a mantisse-exponent decimal representation of x using anz_dezimal_mantisse decimal digits for
	// the mantisse. If MantisseUndExponentGetrennt!=0 is passed f_to_decimalExp() will generate different
	// strings for the mantisse and the exponent. If you assign
	//      char *str=f_to_decimalExp(x, anz_mts, 1, 0)
	// then str points to the mantisse string and str+strlen(str) points to the exponent string.
	// If the pointer ExponentBasis10 passed to f_to_decimalExp() is nonzero, the function will
	// assign the 10-exponent to *ExponentBasis10 ; e.g. if the decimal representation of x
	// is 1.234E58 then the integer 58 is assigned to *ExponentBasis10.

	// *** ATTENTION *** : The functions f_to_decimalExp() and f_to_string() return pointers to static memory
	// containing the decimal representation of the float64 passed to these functions. The string contained
	// in this memory will become invalid if one of the functions f_to_decimalExp(), f_to_string(),
	// f_exp(), f_log(), f_sin(), f_cos(), f_tan(), f_arcsin(), f_arccos(), f_arctan() is called as
	// these functions will overwrite the memory.

	uint8_t f_sign;
	uint8_t len, posm, i;
	int16_t f_ex;
	uint64_t w, w2;
	int16_t Exp10;

	if(anz_dezimal_mantisse>17)
		anz_dezimal_mantisse=17;
	if(anz_dezimal_mantisse<1)
		anz_dezimal_mantisse=1;
	f_split64(&x, &f_sign, &f_ex, &w, 11);
	if(0==f_ex) // Alle denormalisierten Zahlen werden als Null interpretiert.
		{ TemporaryMemory[0]='0'; TemporaryMemory[1]=0; return TemporaryMemory; }
#ifdef F_ONLY_NAN_NO_INFINITY
	if(2047==f_ex)
		{ strcpy(TemporaryMemory, "NaN"); return TemporaryMemory; }
#else
	if(2047==f_ex)
	{
		if(0!=w)
			{ strcpy(TemporaryMemory, "NaN"); return TemporaryMemory; }
		TemporaryMemory[0]=f_sign ? '-' : '+';
		strcpy(TemporaryMemory+1, "INF");
		return TemporaryMemory;
	}
#endif
	f_ex-=1023; // Nach der Abfrage auf 0==f_ex und 2047==f_ex !
	len=0;
	if(f_sign)
		TemporaryMemory[len++]='-';

	if(f_ex >= 0)
		Exp10=(uint16_t)((((uint16_t)f_ex)*10+31)>>5);
	else
		Exp10=(int16_t)(-((((uint16_t)(-f_ex))*9)>>5));

	f_ex+=f_10HochN(-Exp10, &w2);
	w=approx_high_uint64_word_of_uint64_mult_uint64(&w, &w2, 0);
	f_ex+=1-f_shift_left_until_bit63_set(&w);

	while(f_ex<0)
	{
		w=approx_high_uint64_word_of_uint64_mult_uint64_pbv_y(&w, ((uint64_t)10)<<60, 0);
		f_ex+=4-f_shift_left_until_bit63_set(&w);
		Exp10--;
	}
	while(f_ex>=4 || (f_ex==3 && (w & 0xf000000000000000)>=0xa000000000000000))
	{
		w=approx_high_uint64_word_of_uint64_mult_uint64_pbv_y(&w, 0xcccccccccccccccc, 0);
		f_ex-=3+f_shift_left_until_bit63_set(&w);
		Exp10++;
	}
	posm=len;
	++len; // Platz f�r .
	while(0!=(anz_dezimal_mantisse--))
	{
		TemporaryMemory[len]='0';
		if(f_ex>=0)
		{
			TemporaryMemory[len] += (w>>(63-f_ex));
			w <<= 1+f_ex;
			f_ex=-1;
		}
		++len;
		w=approx_high_uint64_word_of_uint64_mult_uint64_pbv_y(&w, ((uint64_t)10)<<60, 0);
		f_ex+=4-f_shift_left_until_bit63_set(&w);
	}
	if(f_ex>=0 && (w>>(63-f_ex)) >= 5)
	{
		for(i=len ; --i>posm ; )
			if(TemporaryMemory[i]=='9')
				TemporaryMemory[i]='0';
			else
			{
				++TemporaryMemory[i];
				break;
			}
		if(i==posm)
		{
			++Exp10;
			TemporaryMemory[++i]='1';
			while(++i<len)
				TemporaryMemory[i]='0';
		}
	}
	TemporaryMemory[posm]=TemporaryMemory[posm+1];
	TemporaryMemory[posm+1]='.';
	if(MantisseUndExponentGetrennt)
		TemporaryMemory[len++]=0;
	TemporaryMemory[len++]='E';
	if(Exp10>0)
		TemporaryMemory[len++]='+';
	itoa(Exp10, &TemporaryMemory[len], 10);
	if(0!=ExponentBasis10)
		*ExponentBasis10=Exp10;
	return TemporaryMemory;
}
#endif

#ifdef F_WITH_to_string
char *f_to_string(float64_t x, uint8_t max_nr_chars, uint8_t max_leading_mantisse_zeros)
{	// f_to_decimalExp() converts the float64 to the decimal representation of the number x if x is
	// a real number or to the strings "+INF", "-INF", "NaN". If x is real, f_to_decimalExp() generates
	// a decimal representation without or with mantisse-exponent representation depending on
	// what in more suitable or possible. If -1 < x < 1 the non-exponent representation is chosen if
	// there are less than max_leading_mantisse_zeros zeros after the decimal point.
	// In most cases f_to_string() will generate a string with maximal 'max_nr_chars' chars. If
	// necessary, f_to_string() reduces the number of decimal digits in the mantisse in order to
	// get a maximum string width of 'max_nr_chars' chars. If however max_nr_chars is so small that
	// even a mantisse of one digit and the corresponding exponent doesn't fit into 'max_nr_chars' chars,
	// the string returned will be longer than 'max_nr_chars' chars.

	// *** ATTENTION *** : The functions f_to_decimalExp() and f_to_string() return pointers to static memory
	// containing the decimal representation of the float64 passed to these functions. The string contained
	// in this memory will become invalid if one of the functions f_to_decimalExp(), f_to_string(),
	// f_exp(), f_log(), f_sin(), f_cos(), f_tan(), f_arcsin(), f_arccos(), f_arctan() is called as
	// these functions will overwrite the memory.

	int16_t exp10;
	int8_t nrd=(0!=(x & 0x8000000000000000)) ? (max_nr_chars-1) : max_nr_chars; // nrd: Zahl der zu berechnenden dezimalen Mantissestellen.
	int8_t nrd_vor=nrd+1;
	char *r=0;
	uint8_t j, k;
	for(j=0; j<3 && nrd!=nrd_vor; j++)
	{
		if(nrd>nrd_vor)
		{
			r=f_to_decimalExp(x, nrd_vor, 1, &exp10); // Nur exp10 berechnen
			break;
		}
		r=f_to_decimalExp(x, nrd, 1, &exp10); // Nur exp10 berechnen
		if(((x>>52)&2047)==2047 || 0==(x & 0x7ff0000000000000))
			return r;
		if(exp10<-max_leading_mantisse_zeros-1)
			break;
		nrd_vor=nrd;

		nrd=f_getsign(x) ? max_nr_chars-1 : max_nr_chars;
			// Diese Variable nrd muss sowohl hier als oben vor Beginn der Schleife initialisiert werden.
		if(exp10<0)
			nrd += exp10-1; // exp10-1 Zeichen durch 0.00 .... verbraucht.
		else if(exp10+1<nrd) // && exp10>=0 .
			--nrd; // Ein Zeichen durch Dezimalpunkt verbraucht.
				// Beispiel f�r den Fall exp10==max_nr_chars-2 : Es sei max_nr_chars=4, exp10=2, Zahl=683.79426
				// ==> Es wird 684 dargestellt
				// (3 statt 4 Zeichen belegt, weil 683.8 zu viele Zeichen belegen w�rde).
	}

	if(0!=(x & 0x7ff0000000000000) && exp10>=-max_leading_mantisse_zeros && nrd>exp10)
	{		// Darstellung ohne 10er-Exponent (E)
		if(f_getsign(x))
			++r;  // - Zeichen
		if(exp10<0)
		{
			for(j=strlen(r) ; (j--)>0 && '0'==r[j] ; )
				r[j]=0;
			r[1]=r[0];
			for(++j ; j>0 ; j--)
				r[j-exp10]=r[j];
			r[0]='0';
			r[1]='.';
			for(j=2; ++exp10<0; )
				r[j++]='0';
		}
		else
		{
			for(j=1 ; j<=exp10 ; j++)
				 if(0==(r[j]=r[j+1]))
				 {
					 while(j<=exp10)
						 r[j++]='0';
					 r[j]=0;
					 break;
				 }
			if(j+1<max_nr_chars && 0!=r[j])
			{
				r[j]='.';
				for(j=strlen(r) ; (j--)>0 && '0'==r[j] ; )
					r[j]=0;
				if('.'==r[j])
					r[j]=0;
			}
			else
				r[j]=0; // Notwendig f�r den Fall j+1>=max_nr_chars
		}
		if(f_getsign(x))
			--r;
	}
	else
	{	// Darstellung mit 10er-Exponent (E)
		j=max_nr_chars-4;
		if(f_getsign(x))
			--j;
		exp10=(x>>52)&2047; // exp10 wird hier zur Aufnahme des bin�ren Exponenten "missbraucht"
		if(exp10<1023) --j;
		if(exp10>1023+34 || exp10<1023-34) --j;
		if(j<1) j=1;
		while((r=f_to_decimalExp(x, j, 0, 0)), strlen(r)>(uint8_t)max_nr_chars)
			if(--j<1)
				break;
		for(j=2; 0!=r[j] && 'E'!=r[j] && 'e'!=r[j] ; j++)
			;
		k=j;
		while((--j>=4 || (!f_getsign(x) && j>=3)) && '0'==r[j])
			;
		while(0!=(r[++j]=r[k++]))
			;
	}
	return r;
}
#endif

#if defined(F_WITH_strtod) || defined(F_WITH_atof)
float64_t f_strtod(char *str, char **endptr)
{
	char *s=str;
	uint8_t f_sign=0, point=0;
	int16_t f_ex;
	uint64_t w, w2;
	int16_t Exp10;
	float64_t x;
	while(*s && (*s==' ' || *s=='\t'))
		s++;

	if((s[0]=='n' || s[0]=='N') && (s[1]=='a' || s[1]=='A') && (s[2]=='n' || s[2]=='N'))
		{ s+=3; goto return_NaN; }

	if(*s=='-')
	{
		f_sign=1;
		++s;
	}
	else if(*s=='+')
		++s;

#ifndef F_ONLY_NAN_NO_INFINITY
	if((s[0]=='i' || s[0]=='I') && (s[1]=='n' || s[1]=='N') && (s[2]=='f' || s[2]=='F'))
	{
		if(endptr) *endptr=s+3;
		return f_sign ? float64_MINUS_INFINITY : float64_PLUS_INFINITY;
	}
#endif

	Exp10=0;
	w=0; f_ex=3;
	while(1)
	{
		if(*s=='.')
		{
			++s;
			if(point)
				goto return_NaN;
			point=1;
		}
		if(!(*s>='0' && *s<='9'))
			break;
		if(f_ex<=63-4)
		{
			if(point)
				--Exp10;
			w=approx_high_uint64_word_of_uint64_mult_uint64_pbv_y(&w, ((uint64_t)10)<<60, 0);
			if(w)
				f_ex+=4;
			w += (((uint64_t)(*s))-'0')<<(63-f_ex);
			if(w)
				f_ex-=f_shift_left_until_bit63_set(&w);
		}
		else if(!point)
			++Exp10;
		++s;
	}
	if(*s=='E' || *s=='e')
	{
		Exp10+=atoi(++s);
		if(*s=='-' || *s=='+')
			++s;
		while(*s>='0' && *s<='9')
			++s;
	}

	f_ex+=1+f_10HochN(Exp10, &w2);
	w=approx_high_uint64_word_of_uint64_mult_uint64(&w, &w2, 0);

	f_combi_from_fixpoint(&x, f_sign, 1023-11+f_ex, &w);
	if(endptr) *endptr=s;
	return x;

return_NaN:
	if(endptr) *endptr=s;
	return float64_ONE_POSSIBLE_NAN_REPRESENTATION;
}
#endif
