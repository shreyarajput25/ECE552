void main() {

	int i;
	for(i = 0; i < 1000000 ; ++i ){
		asm(
			"addi $5, $0, 1  \n \t "
			"lw $1, 2($4)    \n \t "
 			"sw $3, 5($4)   \n \t "    //q1 1-Two cycle stall  ,
			"sub $2, $3, $4    \n \t "  //q2 1-Single cycle stall
			"lw $5, 0($1)    \n \t "
			"addi $1, $2, 5  \n \t "   //q1 1-Single cycle stall
			"add $5, $2, $1  \n \t "   //q1 1-Two cycle stall , q2 1- Single cycle stall
			"sw $6, 0($1)    \n \t "
			"sub $4, $5, $2    \n \t "  //q1 1-Single cycle stall
			"lw $1, 1($4)     \n \t "   //q1 1-Two cycle stall , q2 1- Single cycle stall
			"sub $6, $5, $1     \n \t " //q1 1-Two cycle stall
			"lw $1, 1($2)     \n \t "   
			"add $6, $1, $5  \n \t "    //q1 1-Two cycle stall , q2 1-Two cycle stall

			
		
		  );
	}	
}