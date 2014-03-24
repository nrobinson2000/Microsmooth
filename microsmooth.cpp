#include "microsmooth.h"

uint16_t* ms_init(uint8_t algo)
{
    if(algo & SMA) return (uint16_t *)calloc(SMA_LENGTH, sizeof(uint16_t));
    else if(algo & CMA) return NULL; 
    else if(algo & EMA) return NULL;
    else if(algo & SGA) return (uint16_t *)calloc(SGA_LENGTH, sizeof(uint16_t));
    else if(algo & KZF) return (uint16_t *)calloc(KZ_LENGTH, sizeof(uint16_t));
    else if(algo & RDP) return (uint16_t *)calloc(RDP_LENGTH, sizeof(uint16_t));
}

void deinit(uint16_t *ptr)
{
    free(ptr);
}

int sma_filter(int current_value, uint16_t history_SMA[])
{  
    uint16_t sum=0; /*This constrains SMA_LENGTH*/
    uint16_t average=0;
    uint8_t i;

    for(i=1;i<SMA_LENGTH;i++)
    {
	history_SMA[i-1]=history_SMA[i];
    }
    history_SMA[SMA_LENGTH-1]=current_value;
    
    for(i=0;i<SMA_LENGTH;i++)
    {
	sum+=history_SMA[i];
    }
    average=sum/SMA_LENGTH;

    return average;
}

int cma_filter(int current_value, void * ptr)  //Seems useless
{ 
    static uint16_t cumulative_average=0; /*Limiting factor for k*/
    static uint16_t k=0; /*Limiting factor for runtime*/
    
    cumulative_average=(current_value+k*(uint32_t)cumulative_average)/(k+1);
    k++;

    return cumulative_average;
}

int ema_filter(int current_value, void * ptr)
{ 
    static uint16_t exponential_average=current_value;
    
    exponential_average=(EMA_ALPHA*(uint32_t)current_value + (100 - EMA_ALPHA)*(uint32_t)exponential_average)/100;
    return exponential_average;
}


const int16_t coefficients[]={1512,-3780,-840,5040,10080,12012,10080,5040,-840,-3780,1512};
const uint16_t normalization_value=36036;


int sga_filter(int current_value, uint16_t history_SGA[])
{ 
    uint64_t sum=0;
    uint8_t SGA_MID = SGA_LENGTH/2;
    uint8_t i;
    
    for(i=1;i<SGA_LENGTH;i++)
    {
	history_SGA[i-1]=history_SGA[i];
    }
    history_SGA[SGA_LENGTH-1]=current_value;
    
    for(i=-SGA_MID;i<=(SGA_MID);i++)
    {  
	sum+=history_SGA[i+SGA_MID]*coefficients[i+SGA_MID];
    }
    
    history_SGA[SGA_MID]=sum/normalization_value;
    return history_SGA[SGA_MID];
}
    
//int history_RDP[RDP_LENGTH] = {0,};

  
void rdp(int start_index, int end_index, uint16_t history_RDP[])
{
  int a = history_RDP[end_index]-history_RDP[start_index];
  int b = -(end_index-start_index);
  int c = -start_index*a - history_RDP[start_index]*b;
  
  float max_distance = 0;
  int max_index=0;
  float distance = 0;
  
  for( int i=start_index+1;i<end_index;i++)
  {
    distance=(i*a-history_RDP[i]*b+c)/sqrt(a^2+b^2);
    if (distance>max_distance)
    { 
      max_distance=distance;
      max_index=i;
    }
  }
  
  if(max_distance>epsilon)
  { 
      rdp(start_index,max_index, history_RDP[]);
      rdp(max_index,end_index, history_RDP[]);
   } 
   else 
    
   {
    for( int i=start_index+1;i<end_index;i++) 
    {
      history_RDP[i]=(-c-a*i)/b;
    }
   }
}
   
int rdp_filter(int current_value, uint16_t history_RDP[])
{ 
  uint16_t update_value=history_RDP[0];
  for(int i=1;i<RDP_LENGTH;i++)
  { 
    history_RDP[i-1]=history_RDP[i];  
  }
  
  history_RDP[RDP_LENGTH-1]=current_value;
  
  rdp(0,RDP_LENGTH-1); /*Recursive one*/
  //rdp_iter()
  /*
  int stack[2][RDP_lENGTH/2], top, i=0, j=RDP_LENGTH-1;      
top++;
stack[0][top]=0;
stack[1][top]=RDP_LENGTH-1;

 while(top>-1)
 {
   start_index = stack[0][top];
   end_index = stack[1][top];
top--;

  int a = history_RDP[end_index]-history_RDP[start_index];
  int b = -(end_index-start_index);
  int c = -start_index*a - history_RDP[start_index]*b;
  
  float max_distance = epsilon;
  int max_index=-1;
  float distance = 0;
  
  for( int i=start_index+1;i<end_index;i++)
  {
    distance=(i*a-history_RDP[i]*b+c)/sqrt(a^2+b^2);
    if (distance>max_distance)
    { 
      max_distance=distance;
      max_index=i;
    }
  }
  
  if(max_index!=-1)
  {
      top++;
      stack[0][top]=start_index;
      stack[1][top]=max_index;
      top++;
      stack[0][top]=max_index;
      stack[1][top]=end_index;
   } 
   else 
   {
    for( int i=start_index+1;i<end_index;i++) 
    {
      history_RDP[i]=(-c-a*i)/b;
    }
   }


   */
  return update_value;
}


int history_KZ[KZ_history_LENGTH] = {0,};
int KZ_MID=(KZ_history_LENGTH)/2;
const long coefficients_k[][17]={{0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0},
                                 {0,0,0,0,1,2,3,4,5,4,3,2,1,0,0,0,0},
                                 {0,0,1,3,6,10,15,18,19,18,15,10,6,3,1,0,0},
                                 {1,4,10,20,35,52,68,80,85,80,68,52,35,20,10,4,1}};
//const long coefficients_k3[]={1,3,6,10,15,18,19,18,15,10,6,3,1};
//const long coefficients_k4[]={1,4,10,20,35,52,68,80,85,80,68,52,35,20,10,4,1};

int kz_filter(int current_value)
{ 
  long divisor=1;
  long updated_value=0; 
  for(int i=1;i<KZ_history_LENGTH;i++)
  {
         history_KZ[i-1]=history_KZ[i];
  }
  
  history_KZ[KZ_history_LENGTH-1]=current_value;
    
  for(int k=1;k<=3;k++)
  {  
    
    for(int i=-k*(KZ_LENGTH-1)/2;i<=k*(KZ_LENGTH-1)/2;i++)
    {
      updated_value+=history_KZ[i+KZ_MID]*coefficients_k[k-1][i+KZ_MID];
    } 
    divisor*=KZ_LENGTH;
    updated_value/=divisor;
    history_KZ[KZ_MID]=updated_value;
   
    updated_value=0;

  } 
   
  return history_KZ[KZ_MID];
}
