/* WARNING:  These values are very important, as used under the "default" case. */
#define INT_PART 3
#define DEC_PART 2

double Str2LatLong(char* coord)
//double LLStr::Str2LL(char* coord)
{
    int sign = +1;
    double val;

    int i = 0;  /* an index into coord, the text-input string, indicating the character currently being parsed */

    int p[9] = {0,0,1,  /* degrees */
                0,0,1,  /* minutes */
                0,0,1   /* seconds */
               };
    int* ptr = p;   /* p starts at Degrees. 
                       It will advance to the Decimal part when a decimal-point is encountered,
                       and advance to minutes & seconds when a separator is encountered */
    int  flag = INT_PART; /* Flips back and forth from INT_PART and DEC_PART */

    while(1)
    {
        switch (coord[i])
        {
            /* Any digit contributes to either degrees,minutes, or seconds */
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                *ptr = 10* (*ptr) + (coord[i] - '0');
                if (flag == DEC_PART)  /* it'd be nice if I could find a clever way to avoid this test */
                {
                    ptr[1] *= 10;
                }
                break;

            case '.':     /* A decimal point implies ptr is on an integer-part; advance to decimal part */
                flag = DEC_PART; /* after encountering a decimal point, we are now processing the Decimal Part */
                ptr++;  /* ptr[0] is now the Decimal piece; ptr[1] is the Denominator piece (powers of 10) */
                break;

            /* A Null terminator triggers return (no break necessary) */
            case '\0':
                val = p[0]*3600 + p[3]*60 + p[6];             /* All Integer math */
                if (p[1]) val += ((double)p[1]/p[2]) * 3600;  /* Floating-point operations only if needed */
                if (p[4]) val += ((double)p[4]/p[5]) *   60;  /* (ditto) */
                if (p[7]) val += ((double)p[7]/p[8]);         /* (ditto) */
                return sign * val / 3600.0;                 /* Only one floating-point division! */

            case 'W':
            case 'S':
                sign = -1;
                break;

            /* Any other symbol is a separator, and moves ptr from degrees to minutes, or minutes to seconds */
            default:
                /* Note, by setting DEC_PART=2 and INT_PART=3, I avoid an if-test. (testing and branching is slow) */
                ptr += flag;
                flag = INT_PART; /* reset to Integer part, we're now starting a new "piece" (degrees, min, or sec). */
        }
        i++;
    }

    return -1.0;  /* Should never reach here! */
}
