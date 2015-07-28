#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h> //getline()
#include <string.h> //strtok()
#include <stdbool.h>	//boolean

#define MAX_STR_LEN 1024

typedef struct _readLine{
    int 	cpu;
    double 	sTime;
    char 	rwbs[4];
    char 	action[4];
    long 	sSector;
    int 	size;
    double 	eTime;
} readLine;

void main(void)
{
    FILE *fpR1 = NULL;
    FILE *fpR2 = NULL;
    FILE *fpW = NULL;
    char *line1 = NULL;
    char *line2 = NULL;
    char *tmp = NULL;
    size_t len = 0;
    readLine string1;
    readLine string2;
    char *ptr;
    int cnt1 = 0;
    int cnt2 = 0;
    bool find = false;

    fpR1 = fopen("./trace", "r");
    fpR2 = fopen("./trace", "r");
    fpW = fopen("./trace_p", "a+");

    line1 = malloc(sizeof(char)*MAX_STR_LEN);
    line2 = malloc(sizeof(char)*MAX_STR_LEN);
    tmp = malloc(sizeof(char)*MAX_STR_LEN);

    while( getline(&line1, &len, fpR1) != -1 ){
	printf("%s", line1);
	//line1 changes during strtok.....;;;;
	strncpy(tmp, line1, strlen(line1)+1);
	
	ptr = strtok(tmp, ",");
	string1.cpu = atoi(ptr);
	cnt1++;
	while(ptr != NULL){
	    //printf("%s ",ptr);
	    ptr = strtok(NULL, ",");
	    
	    switch(cnt1){
		case 1:
		    string1.sTime = atof(ptr);
		    break;
		case 2:
		    strncpy(string1.rwbs, ptr, strlen(ptr)+1);
		    break;
		case 3:
		    strncpy(string1.action, ptr, strlen(ptr)+1);
		    break;
		case 4:
		    string1.sSector = atol(ptr);
		    break;
		case 5:
		    string1.size = atoi(ptr);
		    break;
		default:
		    break;
	    }
	    cnt1++;
	}
	cnt1 = 0;

	printf("string1  cpu:%d, sTime:%lf, rwbs:%s, action:%s, sSector:%ld, size:%d\n",
		string1.cpu,
		string1.sTime,
		string1.rwbs,
		string1.action,
		string1.sSector,
		string1.size);
	
	//Only "D" action is eligible
	if( strcmp(string1.action,"D") == 0 ){
	    while( getline(&line2, &len, fpR2) != -1 ){
		//printf("%s", line2);

		ptr = strtok(line2, ",");
		string2.cpu = atoi(ptr);
		cnt2++;
		while(ptr != NULL){
		    //printf("%s ",ptr);
		    ptr = strtok(NULL, ",");

		    switch(cnt2){
			case 1:
			    string2.sTime = atof(ptr);
			    break;
			case 2:
			    strncpy(string2.rwbs, ptr, strlen(ptr)+1);
			    break;
			case 3:
			    strncpy(string2.action, ptr, strlen(ptr)+1);
			    break;
			case 4:
			    string2.sSector = atol(ptr);
			    break;
			case 5:
			    string2.size = atoi(ptr);
			    break;
			default:
			    break;
		    }
		    cnt2++;
		}
		cnt2 = 0;

		/*printf("String2  cpu:%d, sTime:%lf, rwbs:%s, action:%s, sSector:%ld, size:%d\n",
			string2.cpu,
			string2.sTime,
			string2.rwbs,
			string2.action,
			string2.sSector,
			string2.size);*/

		//Succeed to find
		if( string1.sSector == string2.sSector && \
			strcmp(string2.action,"C") == 0 ){
		    printf("Succeed, line:%s\n", line2);
		    find = true;
		    break;
		}
	    }//End of while()
	    //Fail to find
	    if(find == false){
		printf("Fail to find corresponding string\n");
	    }
	    //Write back
	    printf("\n");
	    memset(tmp, 0, sizeof(tmp));
	    if(find == true){
		sprintf(tmp, "%s%lf\n", line1, string2.sTime - string1.sTime);
		printf("************ %s", tmp);
    	    	fwrite(tmp, 1, strlen(tmp)+1, fpW);
	    }else{
		sprintf(tmp, "%s%s\n", line1, "Error");
		printf("************ %s\n", tmp);
    	    	fwrite(tmp, 1, strlen(tmp)+1, fpW);
	    }
	    //Re-initialize
	    find = false;
	    fseek(fpR2, 0, SEEK_SET);
	    memset(tmp, 0, sizeof(tmp));
	} //End of if()
    }
    printf("\nThe END\n");
    
    fclose(fpR1);
    fclose(fpR2);
    fclose(fpW);
    free(line1);
    free(line2);
}

