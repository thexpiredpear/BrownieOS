#include <limits.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

char* itoa(int i, char* str, int base) {
	char* strptr = str;
	char* strreverse = str;
	char* chars = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz";
	if (base < 2 || base > 36) {
		*str = '\0';
		return str;
	}
	if (i < 0 && base == 10) {
		strptr[0] = '-';
		strptr+=1;
	}
	do {
		strptr[0] = chars[35+(i%base)];
		strptr+=1;
		i /= base;
	} while(i);
	strptr[0] = '\0';
	strptr-=1;
	while(strreverse<strptr) {
		char swap = *strreverse;
		strreverse[0] = *strptr;
		strptr[0] = swap;
		strreverse+=1;
		strptr-=1;
	}
	return str;
}

char* utoa(unsigned int i, char* str, int base) {
	char* strptr = str;
	char* strreverse = str;
	char* chars = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz";
	if (base < 2 || base > 36) {
		*str = '\0';
		return str;
	}
	do {
		strptr[0] = chars[35+(i%base)];
		strptr+=1;
		i /= base;
	} while(i);
	strptr[0] = '\0';
	strptr-=1;
	while(strreverse<strptr) {
		char swap = *strreverse;
		strreverse[0] = *strptr;
		strptr[0] = swap;
		strreverse+=1;
		strptr-=1;
	}
	return str;
}

char* ltoa(long i, char* str, int base) {
	char* strptr = str;
	char* strreverse = str;
	char* chars = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz";
	if (base < 2 || base > 36) {
		*str = '\0';
		return str;
	}
	if (i < 0 && base == 10) {
		strptr[0] = '-';
		strptr+=1;
	}
	do {
		strptr[0] = chars[35+(i%base)];
		strptr+=1;
		i /= base;
	} while(i);
	strptr[0] = '\0';
	strptr-=1;
	while(strreverse<strptr) {
		char swap = *strreverse;
		strreverse[0] = *strptr;
		strptr[0] = swap;
		strreverse+=1;
		strptr-=1;
	}
	return str;
}

char* ultoa(unsigned long i, char* str, int base) {
	char* strptr = str;
	char* strreverse = str;
	char* chars = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz";
	if (base < 2 || base > 36) {
		*str = '\0';
		return str;
	}
	do {
		strptr[0] = chars[35+(i%base)];
		strptr+=1;
		i /= base;
	} while(i);
	strptr[0] = '\0';
	strptr-=1;
	while(strreverse<strptr) {
		char swap = *strreverse;
		strreverse[0] = *strptr;
		strptr[0] = swap;
		strreverse+=1;
		strptr-=1;
	}
	return str;
}

char* lltoa(long long i, char* str, int base) {
	char* strptr = str;
	char* strreverse = str;
	char* chars = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz";
	if (base < 2 || base > 36) {
		*str = '\0';
		return str;
	}
	if (i < 0 && base == 10) {
		strptr[0] = '-';
		strptr+=1;
	}
	do {
		strptr[0] = chars[35+(i%base)];
		strptr+=1;
		i /= base;
	} while(i);
	strptr[0] = '\0';
	strptr-=1;
	while(strreverse<strptr) {
		char swap = *strreverse;
		strreverse[0] = *strptr;
		strptr[0] = swap;
		strreverse+=1;
		strptr-=1;
	}
	return str;
}

char* ulltoa(unsigned long long i, char* str, int base) {
	char* strptr = str;
	char* strreverse = str;
	char* chars = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz";
	if (base < 2 || base > 36) {
		*str = '\0';
		return str;
	}
	do {
		strptr[0] = chars[35+(i%base)];
		strptr+=1;
		i /= base;
	} while(i);
	strptr[0] = '\0';
	strptr-=1;
	while(strreverse<strptr) {
		char swap = *strreverse;
		strreverse[0] = *strptr;
		strptr[0] = swap;
		strreverse+=1;
		strptr-=1;
	}
	return str;
}

int vprintf(const char* restrict format, va_list parameters) {

	int written = 0;

	while (*format != '\0') {
		size_t remaining = INT_MAX - written;
		if (*format == '%') {
			format++;
			switch (*format) {
				default:
				case '%': {
					format++;
					char c = (char) '%';
					if (!remaining) {
						// TODO: Set errno to EOVERFLOW
						return -1;
					}
					putchar(c);
					written++;
					break;
				}
				case 'l': {
					format++;
					switch (*format) {
						default:
						case 'l': {
							format++;
							switch (*format) {
								default:
								case 'd': {
									format++;
									long long i = va_arg(parameters, long long);
									char str[21];
									lltoa(i, str, 10);
									size_t len = strlen(str);
									if (remaining < len) {
										// TODO: Set errno to EOVERFLOW
										return -1;
									}
									for (size_t i = 0; i < len; i++) {
										if (putchar(str[i]) == EOF) {
											return -1;
										}
										written++;
									}
									break;
								}
								case 'u': {
									format++;
									unsigned long long i = va_arg(parameters, unsigned long long);
									char str[21];
									ulltoa(i, str, 10);
									size_t len = strlen(str);
									if (remaining < len) {
										// TODO: Set errno to EOVERFLOW
										return -1;
									}
									for (size_t i = 0; i < len; i++) {
										if (putchar(str[i]) == EOF) {
											return -1;
										}
										written++;
									}
									break;
								}
								case 'x': {
									format++;
									unsigned long long i = va_arg(parameters, unsigned long long);
									char str[17];
									ulltoa(i, str, 16);
									size_t len = strlen(str);
									if (remaining < len) {
										// TODO: Set errno to EOVERFLOW
										return -1;
									}
									for (size_t i = 0; i < len; i++) {
										if (putchar(str[i]) == EOF) {
											return -1;
										}
										written++;
									}
									break;
								}
							}
							break;
						}
						break;
					}	
				}
				case 'c': {
					format++;
					char c = (char) va_arg(parameters, int);
					if (!remaining) {
						// TODO: Set errno to EOVERFLOW
						return -1;
					}
					putchar(c);
					written++;
					break;
				}
				case 's': {
					format++;
					const char* str = va_arg(parameters, const char*);
					size_t len = strlen(str);
					if (remaining < len) {
						// TODO: Set errno to EOVERFLOW
						return -1;
					}
					for (size_t i = 0; i < len; i++) {
						if (putchar(str[i]) == EOF) {
							return -1;
						}
						written++;
					}
					break;
				}
				case 'd': {
					format++;
					const int i = va_arg(parameters, const int);
					char str[33];
					itoa(i, str, 10);
					size_t len = strlen(str);
					if (remaining < len) {
						// TODO: Set errno to EOVERFLOW
						return -1;
					} 
					for (size_t i = 0; i < len; i++) {
						if (putchar(str[i]) == EOF) {
							return -1;
						}
						written++;
					}
					break;
				}
				case 'i': {
					format++;
					const int i = va_arg(parameters, const int);
					char str[33];
					utoa(i, str, 10);
					size_t len = strlen(str);
					if (remaining < len) {
						// TODO: Set errno to EOVERFLOW
						return -1;
					} 
					for (size_t i = 0; i < len; i++) {
						if (putchar(str[i]) == EOF) {
							return -1;
						}
						written++;
					}
					break;
				}
				case 'x': {
					format++;
					const int i = va_arg(parameters, const int);
					char str[33];
					utoa(i, str, 16);
					size_t len = strlen(str);
					if (remaining < len) {
						// TODO: Set errno to EOVERFLOW
						return -1;
					}
					for (size_t i = 0; i < len; i++) {
						if (putchar(str[i]) == EOF) {
							return -1;
						}
						written++;
					}
					break;
				}
				case '\0': { 
					return -1;
					break;
				}
			}
		} else {
			if (!remaining) {
				// TODO: Set errno to EOVERFLOW
				return -1;
			}
			putchar(*format);
			format++;
			written++;
		}
	}
	return written;
}

int vsnprintf(char* restrict s, size_t n, const char* restrict format, va_list parameters) {
	int written = 0;
	while (*format != '\0' && written < n - 1) {
		size_t remaining = INT_MAX - written;
		if (*format == '%') {
			format++;
			switch (*format) {
				default:
				case '%': {
					format++;
					char c = (char) '%';
					if (!remaining) {
						// TODO: Set errno to EOVERFLOW
						return -1;
					}
					s[written] = c;
					written++;
					break;
				}
				case 'c': {
					format++;
					char c = (char) va_arg(parameters, int);
					if (!remaining) {
						// TODO: Set errno to EOVERFLOW
						return -1;
					}
					s[written] = c;
					written++;
					break;
				}
				case 's': {
					format++;
					const char* str = va_arg(parameters, const char*);
					size_t len = strlen(str);
					if (remaining < len) {
						// TODO: Set errno to EOVERFLOW
						return -1;
					}
					for (size_t i = 0; i < len && written < (int) n - 1; i++) {
						s[written] = str[i];
						written++;
					}
					break;
				}
				case 'd':
				case 'i': {
					format++;
					const int i = va_arg(parameters, const int);
					char str[33];
					itoa(i, str, 10);
					size_t len = strlen(str);
					if (remaining < len) {
						// TODO: Set errno to EOVERFLOW
						return -1;
					} 
					for (size_t i = 0; i < len && written < (int) n - 1; i++) {
						s[written] = str[i];
						written++;
					}
					break;
				}
				case 'x': {
					format++;
					const int i = va_arg(parameters, const int);
					char str[33];
					utoa(i, str, 16);
					size_t len = strlen(str);
					if (remaining < len) {
						// TODO: Set errno to EOVERFLOW
						return -1;
					}
					for (size_t i = 0; i < len && written < (int) n - 1; i++) {
						s[written] = str[i];
						written++;
					}
					break;
				}
				case '\0': { 
					return -1;
					break;
				}
			}
		} else {
			if (!remaining) {
				// TODO: Set errno to EOVERFLOW
				return -1;
			}
			s[written] = *format;
			format++;
			written++;
		}
	}
	s[written] = '\0';
	return written;
}

int vsprintf(char* restrict s, const char* restrict format, va_list parameters) {
	return vsnprintf(s, INT_MAX, format, parameters);
}

int printf(const char* restrict format, ...) {
	va_list parameters;
	va_start(parameters, format);
	int written = vprintf(format, parameters);
	va_end(parameters);
	return written;
}

int snprintf(char* restrict s, size_t n, const char* restrict format, ...) {
	va_list parameters;
	va_start(parameters, format);
	int written = vsnprintf(s, n, format, parameters);
	va_end(parameters);
	return written;
}

int sprintf(char* restrict s, const char* restrict format, ...) {
	va_list parameters;
	va_start(parameters, format);
	int written = vsprintf(s, format, parameters);
	va_end(parameters);
	return written;
}
