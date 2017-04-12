#include <stdio.h>

int main(int argc, char* argv[])
{
	char* str = "Helloworld\n"; 
	printf("%s", str); 
	FILE* pFile = NULL;

	pFile =fopen("C://work//output.txt","wb+");
	
	if( !pFile)
	{
		printf("Can't open file output.txt\n"); 
		return -1; 
	}

	fwrite(str, 1, 11, pFile); 
	
	printf("TestCase\n"); 
	fclose(pFile); 

	return 0; 
}