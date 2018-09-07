#include<stdio.h>
int main()
{
char b[100];
char* s=b;
unsigned int c=0x32046301;
sprintf(s,"%.8x",c);
printf("%s",s);
return 0;
}
