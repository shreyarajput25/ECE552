void main(){
  
  int i;

  int array[50000] = {0};
  for(i = 0; i < 50000; i= i + 16){
     array[i] = array[i];
  }
}