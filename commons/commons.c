#include <linux/kernel.h>
#include <linux/math64.h>

#include "commons.h"

static void _itoa(int64_t value, char *buf, int base) {
	char tmp[64 + 1];
	char *tp = tmp;
	uint32_t i;
	uint64_t v;
	bool sign;
	char *sp;

	if (buf == NULL) {
		return;
	}

	sign = value < 0;
	if (sign) {
		v = -value;
	} else {
		v = (uint64_t) value;
	}

	while (v || tp == tmp) {
		v = div_u64_rem(v, base, &i);
		if (i < 10)
			*tp++ = '0' + i;
		else
			*tp++ = 'a' + i - 10;
	}

	sp = buf;

	if (sign)
		*sp++ = '-';
	while (tp > tmp)
		*sp++ = *--tp;
	*sp = 0;
}

unsigned long long to_usec(struct timespec64 *t) {
	return (t->tv_sec * 1000000) + (t->tv_nsec / 1000);
}

unsigned long long diff_usec(struct timespec64 *t1, struct timespec64 *t2) {
	struct timespec64 diff;
	diff = timespec64_sub(*t2, *t1);
	return to_usec(&diff);
}

char toUpper(char c) {
	if (c >= 97 && c <= 122) {
		return c - 32;
	}
	return c;
}

int valToStr(char *buf, int64_t val, const char *vals, bool sign, uint8_t len,
		uint8_t base) {
	if (vals == NULL) {
		if (base == 0) {
			base = 10;
		}
		if (sign) {
			if (len < 3) {
				if (len == 1) {
					if ((val & 0x80) == 0x80) {
						// negative => add sign bits
						val |= 0xff00;
					}
				}
				_itoa((int16_t) val, buf, base);
			} else {
				if (len == 3) {
					if ((val & 0x800000) == 0x800000) {
						// negative => add sign bits
						val |= 0xff000000;
					}
				}
				_itoa((int32_t) val, buf, base);
			}
		} else {
			_itoa(val, buf, base);
		}
		return sprintf(buf, "%s\n", buf);
	} else {
		if (val > vals[0] - 1) {
			return -EFAULT;
		}
		return sprintf(buf, "%c\n", vals[val + 1]);
	}
}

int64_t strToVal(const char *buf, const char *vals, bool sign, uint8_t base) {
	int i, ret;
	int64_t val;
	char valC;
	if (vals == NULL) {
		if (base == 0) {
			base = 10;
		}
		ret = kstrtoll(buf, base, &val);
		if (ret < 0) {
			return -EINVAL;
		}
	} else {
		val = -1;
		valC = toUpper(buf[0]);
		for (i = 0; i < vals[0]; i++) {
			if (vals[i + 1] == valC) {
				val = i;
				break;
			}
		}
		if (val == -1) {
			return -EINVAL;
		}
	}

	if (!sign && val < 0) {
		return -EINVAL;
	}

	return val & 0xffffffff;
}
