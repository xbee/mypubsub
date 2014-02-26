#include<unistd.h>

int main()
{
char buffer[]="GOOD!!MORNING!!";
system("notify-send buffer");
return 0;
}

