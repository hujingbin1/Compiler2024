%{
#include <cstdio>
#include <cstring>

// 词法分析头文件
#include "FlexLexer.h"

// bison生成的头文件
#include "BisonParser.h"

// 抽象语法树函数定义原型头文件
#include "AST.h"

// LR分析失败时所调用函数的原型声明
void yyerror(char * msg);

%}

%nonassoc LOWER_THAN_ELSE
%nonassoc T_ELSE

// 联合体声明，用于后续终结符和非终结符号属性指定使用
%union {
    class ast_node * node;

    struct digit_int_attr integer_num;
    struct digit_real_attr float_num;
    struct var_id_attr var_id;
	unsigned Exp_type;
	int operator_type;
};

// 文法的开始符号
%start  CompUnit

// 指定文法的终结符号，<>可指定文法属性
// 对于单个字符的算符或者分隔符，在词法分析时可直返返回对应的字符即可
//T_VOID
%token <integer_num> IntConst
%token <float_num> FloatConst
%token <node> T_ID
%token T_RETURN T_ADD T_SUB T_CONST T_BREAK T_CONTINUE T_IF T_WHILE
%token T_MUL T_DIV T_MOD T_NOT T_LT T_LE T_BT T_BE T_EQ T_NE T_AND T_OR T_INT T_FLOAT T_VOID
//T_FUNC T_NOTE

%left T_ADD T_SUB
%left T_MUL T_DIV T_MOD
%left T_OR
%left T_AND
%left T_NOT 



%type <node> CompUnit

// 指定文法的非终结符号，<>可指定文法属性

%type <node> Decl 					//声明

%type <node> ConstDecl				//常量声明
%type <Exp_type> BType					//基本类型
%type <node> ConstDef				//常数定义
%type <node> ConstInitVal			//常量初值
%type <node> ConstDefGp				//常数定义组
%type <node> ConstExpGp				//常数数组
%type <node> ConstExp 				//a[ConstExp] or a = ConstExp
%type <node> ConstInitValGp			//a[][][] = {ConstInitValGp	}
%type <node> dummy1

%type <node> VarDecl				//变量声明
%type <node> VarDef					//变量定义
%type <node> VarDefGp				//变量定义组
%type <node> InitValGp
%type <node> InitVal				//变量初值

%type <node> FuncDecl	  			//函数声明
%type <node> FuncDef				//函数定义
//%type <node> FuncType
%type <node> FuncFParams			//函数形参表
%type <node> FuncFParamGp
%type <node> FuncFParam				//函数形参

%type <node> Block					//语句块
%type <node> BlockItem				//语句块项
%type <node> BlockItemGp
%type <node> Stmt					//语句

%type <node> Exp					//表达式
%type <node> ExpGp
%type <node> Cond					//条件表达式
%type <node> LVal					//左值表达式
%type <node> PrimaryExp				//基本表达式
%type <node> Number					//数值
%type <node> UnaryExp				//一元表达式

%type <operator_type> UnaryOp				//单目运算符
%type <node> FuncRParams			//函数实参表
%type <node> FuncRParamGp
%type <node> MulExp					//乘除模表达式
%type <node> AddExp					//加减表达式
%type <node> RelExp					//关系表达式
%type <node> EqExp					//相等性表达式
%type <node> LAndExp				//逻辑与表达式
%type <node> LOrExp					//逻辑或表达式




%%
CompUnit :
	FuncDef {
		//printf("%d : CompUnit -> FuncDef\n", ++nCount);
		$$ = create_contain_node(ast_operator_type::AST_OP_COMPILE_UNIT,CompUnitK, $1);
        SetNode_ASTType($$,AT_CompUnit) ;
		ast_root = $$;
	}
	| Decl {
		//printf("%d : CompUnit -> Decl\n", ++nCount);
		$$ = create_contain_node(ast_operator_type::AST_OP_COMPILE_UNIT,CompUnitK, $1);
        SetNode_ASTType($$, AT_CompUnit);
		ast_root = $$;
	}
	| CompUnit FuncDef {
		//printf("%d : CompUnit -> CompUnit FuncDef\n", ++nCount);
		$$ = $1;
		insert_ast_node($$,$2);
	}
	| CompUnit Decl {
		//printf("%d : CompUnit -> CompUnit Decl\n", ++nCount);
		$$ = $1;
		insert_ast_node($$,$2);
	}
	;

/*声明语句*/
Decl :
	FuncDecl {
		//printf("%d : Decl -> FuncDecl\n", ++nCount);
    	$$ = $1;
	}
	| ConstDecl {
		//printf("%d : Decl -> ConstDecl\n", ++nCount);
    	$$ = $1;
	}
	| VarDecl {
		//printf("%d : Decl -> VarDecl\n", ++nCount);
    	$$ = $1;
	}
	;

/*常量声明*/
ConstDecl :
	T_CONST BType ConstDefGp ';' {
		//printf("%d : ConstDecl -> const BType ConstDefGp\n", ++nCount);
		$$ = $3;
		SetBasicType($$, (ExpType)$2);
		SetNode_ASTType($$, AT_ConstDecl);
	}
	;

ConstDefGp :
	ConstDefGp ',' ConstDef {
		//printf("%d : ConstDefGroup -> ConstDefGroup \",\" ConstDef\n", ++nCount);
		$$ = $1;
		insert_ast_node($$,$3);
	}
    | ConstDef {
		//printf("%d : ConstDefGroup -> ConstDef\n", ++nCount);
		$$ = new ast_node(ConstDeclK);
		insert_ast_node($$,$1);
	}
    ;

BType :
	T_INT {
		$$ = Integer;
	}
	| T_FLOAT {
		$$ = Float;
	}
	;

ConstDef :
	T_ID ConstExpGp '=' ConstInitVal {
		//printf("%d : ConstDef -> Ident ConstExpGroup \"=\" ConstInitVal\n", ++nCount);
		$$ = $1;
		SetOriginalType($$,ConstDefK);
		SetNode_ASTType($$,AT_ConstDef);
		insert_ast_node($$,$2);
		insert_ast_node($$,$4);
	}
	;

ConstExpGp :
	'[' ConstExp ']' ConstExpGp {
		//printf("%d : ConstExpGroup -> \"[\" ConstExp \"]\" ConstExpGroup\n", ++nCount);
		$$ = $4;
		insert_ast_node($$,$2);
	}
	|{
		//printf("%d : ConstExpGroup -> (null)\n", ++nCount);
		$$ = new ast_node(ConstExpGroupK);
		SetNode_ASTType($$,AT_ConstExpGroup);
	}
	;

ConstInitVal : // a+b | {} | {Gp}
	ConstExp {
		//printf("%d : ConstInitVal -> ConstExp\n", ++nCount);
		$$ = new ast_node(ConstInitValK);
		/*
		if($$ == NULL)
		{
			printf( "Insufficient memory available\n" );
		}
		*/
		SetNode_ASTType($$,AT_ConstInitVal);
		insert_ast_node($$,$1);
	}
	| '{' '}' {
		//printf("%d : ConstInitVal -> \"{\" \"}\"\n", ++nCount);
		$$ = new ast_node(ConstInitValK);
		SetNode_ASTType($$,AT_ConstInitVal);
	}
	| '{' ConstInitValGp '}' {
		//printf("%d : ConstInitVal -> \"{\" ConstInitValGroup \"}\"\n", ++nCount);
		$$ = $2;
		SetOriginalType($$,ConstInitValK);
		SetNode_ASTType($$,AT_ConstInitVal);
	}
	;

ConstInitValGp :
	ConstInitVal dummy1 {
		//printf("%d : ConstInitValGroup -> ConstInitVal dummy1\n", ++nCount);
		$$ = $2;
		insert_ast_node($$,$1);
	}
	;

dummy1 :
	',' ConstInitVal dummy1 {
		//printf("%d : dummy1 -> \",\" ConstInitVal dummy1\n", ++nCount);
		$$ = $3;
		insert_ast_node($$,$2);
	}
	| {
		//printf("%d : dummy1 -> (null)\n", ++nCount);
		$$ = new ast_node(ConstInitValGroupK);
	}
	;

//Var
VarDecl :
	BType VarDef VarDefGp ';' {
		//printf("%d : VarDecl -> BType VarDef VarDefGroup \";\"\n", ++nCount);
		$$ = $3;
		SetNode_ASTType($$,AT_VarDecl);
		SetBasicType($$, (ExpType)$1);
		insert_ast_node($$,$2);
	}
	;

VarDefGp :
    ',' VarDef VarDefGp {
		//printf("%d : VarDefGroup -> \",\" VarDef VarDefGroup\n", ++nCount);
		$$ = $3;
		insert_ast_node($$,$2);
	}
    | {
		//printf("%d : VarDefGroup -> (null)\n", ++nCount);
		$$ = new ast_node(VarDeclK);
	}
	;

VarDef :
	T_ID ConstExpGp {
		//printf("%d : VarDef -> Ident ConstExpGroup\n", ++nCount);
		$$ = $1;
		SetOriginalType($$, VarDefK);
		SetNode_ASTType($$,AT_VarDef);
		insert_ast_node($$,$2);
	}
	| T_ID ConstExpGp '=' InitVal {
		//printf("%d : VarDef -> Ident ConstExpGroup \"=\" InitVal\n", ++nCount);
		$$ = $1;
		SetOriginalType($$, VarDefK);
		SetNode_ASTType($$,AT_VarDef);
		insert_ast_node($$,$2);
		insert_ast_node($$,$4);
	}
	;

InitVal :
	Exp{
		//printf("%d : InitVal -> Exp\n", ++nCount);
		$$ = new ast_node(InitValK);
		SetNode_ASTType($$,AT_InitVal);
		insert_ast_node($$,$1);
	}
	|'{' '}' {
		//printf("%d : InitVal -> \"{\" \"}\"\n", ++nCount);
		$$ = new ast_node(InitValK);
		SetNode_ASTType($$,AT_InitVal);
	}
	| '{' InitVal InitValGp '}' {
		//printf("%d : InitVal -> \"{\" InitVal InitValGroup \"}\"\n", ++nCount);
		$$ = $3;
		SetNode_ASTType($$,AT_InitVal);
		insert_ast_node($$,$2);
	}
    ;

InitValGp :
	',' InitVal InitValGp {
		//printf("%d : InitValGroup -> \",\" InitVal InitValGroup\n", ++nCount);
		$$ = $3;
		insert_ast_node($$,$2);
	}
	| {
		//printf("%d : InitValGroup -> (null)\n", ++nCount);
		$$ = new ast_node(InitValK);
	}
    ;

//Func
FuncDef :
	BType T_ID '(' ')' Block {
		//printf("%d : FuncDef -> BType Ident \"(\" \")\" Block\n", ++nCount);
		$$ = $2 ;
		SetOriginalType($$, FuncDefK);
		SetNode_ASTType($$,AT_FuncDef);
		SetBasicType($$, (ExpType)$1);

		$2 =  new ast_node(FuncFParamsK);
		SetNode_ASTType($2,AT_FuncFParams);
		insert_ast_node($$,$2);
		insert_ast_node($$,$5);
	}
	| BType T_ID '(' FuncFParams ')' Block {
		//printf("%d : FuncDef -> BType Ident \"(\" FuncFParams \")\" Block\n", ++nCount);
		$$ = $2 ;
		SetOriginalType($$, FuncDefK);
		SetNode_ASTType($$,AT_FuncDef);
		SetBasicType($$, (ExpType)$1);

		insert_ast_node($$,$4);
		insert_ast_node($$,$6);
	}
	| T_VOID T_ID '(' ')' Block {
		//printf("%d : FuncDef -> \"void\" Ident \"(\" \")\" Block\n", ++nCount);
		$$ = $2 ;
		SetOriginalType($$, FuncDefK);
		SetNode_ASTType($$,AT_FuncDef);
		SetBasicType($$, (ExpType)1); //Void = 1

		$2 =  new ast_node(FuncFParamsK);
		SetNode_ASTType($2,AT_FuncFParams);
		insert_ast_node($$,$2);
		insert_ast_node($$,$5);
	}
	| T_VOID T_ID '(' FuncFParams ')' Block {
		//printf("%d : FuncDef -> \"void\" Ident \"(\" FuncFParams \")\" Block\n", ++nCount);
		$$ = $2 ;
		SetOriginalType($$, FuncDefK);
		SetNode_ASTType($$,AT_FuncDef);
		SetBasicType($$, (ExpType)1);

		insert_ast_node($$,$4);
		insert_ast_node($$,$6);
	}
    ;


FuncFParams :
    FuncFParam FuncFParamGp {
		//printf("%d : FuncFParams -> FuncFParam FuncFParamGroup\n", ++nCount);
		$$ = $2;
        insert_ast_node($$,$1);
	}
	;

FuncFParamGp :
	',' FuncFParam FuncFParamGp{
		//printf("%d : FuncFParamGroup -> \",\" FuncFParam FuncFParamGroup\n", ++nCount);
		$$ = $3;
        insert_ast_node($$,$2);
	}
	| {
		//printf("%d : FuncFParamGroup -> (null)\n", ++nCount);
		$$ = new ast_node(FuncFParamsK);
		SetNode_ASTType($$,AT_FuncFParams);
	}
    ;

FuncFParam :
	BType {
		//printf("%d : FuncFParam -> BType\n", ++nCount);
		$$ = new ast_node(FuncFParamK,yylineno); //?**************************************************************
		SetNode_ASTType($$,AT_FuncFParam);
		SetBasicType($$, (ExpType)$1);
	}
	| BType T_ID {
		//printf("%d : FuncFParam -> BType Ident\n", ++nCount);
		$$ = $2;
		SetOriginalType($$,FuncFParamK);
		SetNode_ASTType($$,AT_FuncFParam);
		SetBasicType($$, (ExpType)$1);
	}
	| BType T_ID '[' ']' ExpGp {
		ast_node * temp;
		int tempValue;
		//printf("%d : FuncFParam -> BType Ident \"[\" \"]\" ExpGroup\n", ++nCount);
		$$ = $5;
		SetOriginalType($$, FuncFParamK);
		SetNode_ASTType($$,AT_FuncFParam);
		SetBasicType($$, (ExpType)$1);
		SetLine_no($$, GetLine_no($2));
		SetVarName($$,GetVarName($2));
		free_ast_node($2); //这里把它的所有子节点都释放了

        temp = new ast_node(ExpK);
		SetNode_ASTType(temp,AT_Expression);
		SetBasicType(temp, Integer);
		tempValue = 0;
		SetLiteralContent(temp, &tempValue, Integer);
		insert_ast_node($$,temp);// 插入一个空孩子，代表数组
	}
	;

ExpGp :
	'[' Exp']' ExpGp {
		//printf("%d : ExpGroup -> \"[\" Exp \"]\" ExpGroup\n", ++nCount);
		$$ = $4;
		insert_ast_node($$,$2);
	}
	| {
		//printf("%d : ExpGroup -> (null)\n", ++nCount);
		$$ = new ast_node();
	}
	;

Block :
	'{' BlockItemGp'}' {
		//printf("%d : Block -> \"{\" BlockItemGroup \"}\"\n", ++nCount);
		$$ = $2;
		SetNode_ASTType($$,AT_Block);
	}
	;

BlockItemGp :
    BlockItem BlockItemGp {
		//printf("%d : BlockItemGroup -> BlockItem BlockItemGroup\n", ++nCount);
		$$ = $2;
		insert_ast_node($$,$1);
	}
    | {
		//printf("%d : BlockItemGroup -> (null)\n", ++nCount);
		$$ = new ast_node(BlockK);
	}
	;

BlockItem :
	Decl {
		//printf("%d : BlockItem -> Decl\n", ++nCount);
		$$ = $1;
	}
	| Stmt {
		//printf("%d : BlockItem -> Stmt\n", ++nCount);
		$$ = $1;
	}
    ;

Stmt :
	LVal '=' Exp ';' {
		//printf("%d : Stmt -> LVal \"=\" Exp \";\"\n", ++nCount);
		$$ = new ast_node(StmtK);
		SetNode_ASTType($$,AT_AssignStmt);
		insert_ast_node($$,$1);
		insert_ast_node($$,$3);

	}
	| ';' {
		//printf("%d : Stmt -> \";\"\n", ++nCount);
		$$ = new ast_node(StmtK);
		SetNode_ASTType($$, AT_EmptyStmt);
	}
	| Exp ';' {
		//printf("%d : Stmt -> Exp \";\"\n", ++nCount);
		$$ = $1;
	}
	| Block {
		//printf("%d : Stmt -> Block\n", ++nCount);
		$$ = $1;
		SetNode_ASTType($$, AT_Block);
	}
	| T_IF '(' Cond ')' Stmt %prec LOWER_THAN_ELSE {
		// printf("%d : Stmt -> \"if\" \"(\" Cond \")\" Stmt\n", ++nCount);
		$$ = new ast_node(StmtK);
		SetNode_ASTType($$,AT_IfStmt);
		insert_ast_node($$,$3);
		insert_ast_node($$,$5);
	}
	| T_IF '(' Cond ')' Stmt T_ELSE Stmt {
		// printf("%d : Stmt -> \"if\" \"(\" Cond \")\" Stmt \"else\" Stmt\n", ++nCount);
		$$ = new ast_node(StmtK);
		SetNode_ASTType($$,AT_IfStmt);
		insert_ast_node($$,$3);
		insert_ast_node($$,$5);
		insert_ast_node($$,$7);
	}
	| T_WHILE '(' Cond ')' Stmt {
		//printf("%d : Stmt -> \"while\" \"(\" Cond \")\" Stmt\n", ++nCount);
		$$ = new ast_node(StmtK);
		SetNode_ASTType($$,AT_WhileStmt);
		insert_ast_node($$,$3);
		insert_ast_node($$,$5);
	}
	| T_BREAK ';' {
		//printf("%d : Stmt -> \"break\" \";\"\n", ++nCount);
		$$ = new ast_node(StmtK);
		SetNode_ASTType($$,AT_BreakStmt);
		SetLine_no($$,yylineno);
	}
    | T_CONTINUE ';' {
		//printf("%d : Stmt -> \"continue\" \";\"\n", ++nCount);
		$$ = new ast_node(StmtK);
		SetNode_ASTType($$,AT_ContinueStmt);
		SetLine_no($$,yylineno);
	}
	| T_RETURN ';' {
		//printf("%d : Stmt -> \"return\" \";\"\n", ++nCount);
		$$ = new ast_node(StmtK);
		SetNode_ASTType($$, AT_ReturnStmt);
		SetLine_no($$,yylineno);
	}
	| T_RETURN Exp ';' {
		//printf("%d : Stmt -> \"return\" \";\"\n", ++nCount);
		$$ = new ast_node(StmtK);
		SetNode_ASTType($$, AT_ReturnStmt);
		SetLine_no($$,yylineno);
		insert_ast_node($$,$2);
	}
	;

Exp :
	AddExp {
		// printf("%d : Exp -> AddExp\n", ++nCount);
		$$ = $1;
	}
	;

Cond :
	LOrExp {
		// printf("%d : Cond -> LOrExp\n", ++nCount);
		$$ = $1;
		SetNode_ASTType($$,AT_Condition);
	}
	;

LVal :
	T_ID ExpGp {
		// printf("%d : LVal -> Ident ExpGroup\n", ++nCount);
		$$ = $2;
		SetOriginalType( $$, LValK);
		SetLine_no($$,GetLine_no($1));
		SetVarName($$,GetVarName($1));

		free_ast_node($1); //这里把它的所有子节点都释放了
	}
	;

Number :
	IntConst {
		// printf("%d : Number -> IntConst\n", ++nCount);
		$$ = new ast_node(NumberK);
		SetBasicType($$, Integer);
		SetLiteralContent($$, &$1, Integer);
	}
	| FloatConst{
		// printf("%d : Number -> floatConst\n", ++nCount);
		$$ = new ast_node(NumberK);
		SetBasicType($$, Float);
		SetLiteralContent($$, &$1, Float);
	}
	;

PrimaryExp :
	'(' Exp ')' {
		// printf("%d : PrimaryExp -> \"(\" Exp \")\"\n", ++nCount);
		$$ = $2;
	}
	| LVal {
		// printf("%d : PrimaryExp -> LVal\n", ++nCount);
		$$ = $1;
	}
	| Number {
		// printf("%d : PrimaryExp -> Number\n", ++nCount);
		$$ = $1;
	}
	;

UnaryExp :
	PrimaryExp {
		// printf("%d : UnaryExp -> PrimaryExp\n", ++nCount);
		$$ = $1;
	}
	| T_ID '(' ')' {
		// printf("%d : UnaryExp -> Ident \"(\" \")\"\n", ++nCount);
		$$ = $1;
		SetOriginalType($$,UnaryExpK);
		SetNode_ASTType($$,AT_FuncCall);
		Ast_SetFlag($$, 123);
		/*#define FUNCCALL_SIGNATURE     0x00010000l     用于解决BUG7 */
	}
	|T_ID '(' FuncRParams ')' {
		// printf("%d : UnaryExp -> Ident \"(\" FuncRParams \")\"\n", ++nCount);
		$$ = $3;
		SetOriginalType($$,UnaryExpK);
		SetNode_ASTType($$,AT_FuncCall);
		SetLine_no($$,GetLine_no($1));
		SetVarName($$,GetVarName($1));
		Ast_SetFlag($$, 123);
		/*#define FUNCCALL_SIGNATURE     0x00010000l     用于解决BUG7 */
		free_ast_node($1);
	}
	| UnaryOp UnaryExp {
		// printf("%d : UnaryExp -> UnaryOp UnaryExp\n", ++nCount);
		$$ = create_contain_node((ast_operator_type)$1,UnaryExpK, $2);
	}
	;

UnaryOp :
    T_ADD {
		//printf("%d : UnaryOp -> \"+\"\n", ++nCount);
		$$ = (int)ast_operator_type::AST_UNARYOP_PLUS;
	}
    | T_SUB {
		//printf("%d : UnaryOp -> \"-\"\n", ++nCount);
		$$ = (int)ast_operator_type::AST_UNARYOP_MINUS;
	}
    | T_NOT {
		//printf("%d : UnaryOp -> \"!\"\n", ++nCount);
		$$ = (int)ast_operator_type::AST_UNARYOP_NOT;
	}//'!'
	;

FuncRParams :
	Exp FuncRParamGp {
		//printf("%d : FuncRParams -> Exp FuncRParamsGroup\n", ++nCount);
		$$ = $2;
		SetNode_ASTType($$,AT_FuncRParams);
		insert_ast_node($$,$1);
	}
	;

FuncRParamGp :
    ',' Exp FuncRParamGp{
		//printf("%d : FuncRParamsGroup -> \",\" Exp FuncRParamsGroup\n", ++nCount);
		$$ = $3;
		insert_ast_node($$,$2);
	}
    | {
		//printf("%d : FuncRParamsGroup -> (null)\n", ++nCount);
		$$ = new ast_node(FuncRParamsK);
	}
    ;

MulExp :
	UnaryExp {
		// printf("%d : MulExp -> UnaryExp\n", ++nCount);
		$$ = $1;
	}
	| MulExp T_MUL UnaryExp {
		//printf("%d : MulExp -> MulExp \"*\" UnaryExp\n", ++nCount);
		$$ = create_contain_node(ast_operator_type::AST_OP_MULT,MulExpK,$1);
		insert_ast_node($$,$3);

	}
	| MulExp T_DIV UnaryExp {
		//printf("%d : MulExp -> MulExp \"/\" UnaryExp\n", ++nCount);
		$$ = create_contain_node(ast_operator_type::AST_OP_DIV,MulExpK,$1);
		insert_ast_node($$,$3);
	}
	| MulExp T_MOD UnaryExp {
		//printf("%d : MulExp -> MulExp \" %% \" UnaryExp\n", ++nCount);
		$$ = create_contain_node(ast_operator_type::AST_OP_MOD,MulExpK,$1);
		insert_ast_node($$,$3);
	}
    ;

AddExp :
	MulExp {
		// printf("%d : AddExp -> MulExp\n", ++nCount);
		$$ = $1;
	}
	| AddExp T_ADD MulExp {
		//printf("%d : AddExp -> AddExp \"+\" MulExp\n", ++nCount);
		$$ = create_contain_node(ast_operator_type::AST_OP_ADD,AddExpK,$1);
		insert_ast_node($$,$3);
	}
	| AddExp T_SUB MulExp {
		//printf("%d : AddExp -> AddExp \"-\" MulExp\n", ++nCount);
		$$ = create_contain_node(ast_operator_type::AST_OP_SUB,AddExpK,$1);
		insert_ast_node($$,$3);
	}
    ;

RelExp :
	AddExp {
		// printf("%d : RelExp -> AddExp\n", ++nCount);
		$$ = $1;
	}
	| RelExp T_LT AddExp {
		// printf("%d : RelExp -> RelExp \"<\" AddExp\n", ++nCount);
		$$ = create_contain_node(ast_operator_type::AST_OP_LT,RelExpK,$1);
		SetNode_ASTType($$,AT_Condition);
		insert_ast_node($$,$3);
	}// <
	| RelExp T_BT AddExp {
		// printf("%d : RelExp -> RelExp \">\" AddExp\n", ++nCount);
		$$ = create_contain_node(ast_operator_type::AST_OP_BT,RelExpK,$1);
		SetNode_ASTType($$,AT_Condition);
		insert_ast_node($$,$3);
	}// >
	| RelExp T_LE AddExp {
		// printf("%d : RelExp -> RelExp \"<=\" AddExp\n", ++nCount);
		$$ = create_contain_node(ast_operator_type::AST_OP_LE,RelExpK,$1);
		SetNode_ASTType($$,AT_Condition);
		insert_ast_node($$,$3);
	}// <=
	| RelExp T_BE AddExp {
		// printf("%d : RelExp -> RelExp \">=\" AddExp\n", ++nCount);
		$$ = create_contain_node(ast_operator_type::AST_OP_BE,RelExpK,$1);
		SetNode_ASTType($$,AT_Condition);
		insert_ast_node($$,$3);
	}// >=
    ;

EqExp :
	RelExp {
		// printf("%d : EqExp -> RelExp\n", ++nCount);
		$$ = $1;
	}
	| EqExp T_EQ RelExp {
		// printf("%d : EqExp -> EqExp \"==\" RelExp\n", ++nCount);
		$$ = create_contain_node(ast_operator_type::AST_OP_EQ,EqExpK,$1);
		SetNode_ASTType($$,AT_Condition);
		insert_ast_node($$,$3);
	} // ==
	| EqExp T_NE RelExp {
		// printf("%d : EqExp -> EqExp \"!=\" RelExp\n", ++nCount);
		$$ = create_contain_node(ast_operator_type::AST_OP_NQ,EqExpK,$1);
		SetNode_ASTType($$,AT_Condition);
		insert_ast_node($$,$3);
	} //!=
    ;

LAndExp :
	EqExp {
		// printf("%d : LAndExp -> EqExp\n", ++nCount);
		$$ = $1;
	}
	| LAndExp T_AND EqExp {
		// printf("%d : LAndExp -> LAndExp \"&&\" EqExp\n", ++nCount);
		$$ = create_contain_node(ast_operator_type::AST_OP_AND,LAndExpK,$1);
		SetNode_ASTType($$,AT_Condition);
		insert_ast_node($$,$3);
	}
    ;

LOrExp :
    LAndExp {
		// printf("%d : LOrExp -> LAndExp\n", ++nCount);
		$$ = $1;
	}
    | LOrExp T_OR LAndExp {
		// printf("%d : LOrExp -> LOrExp \"||\" LAndExp\n", ++nCount);
		$$ = create_contain_node(ast_operator_type::AST_OP_OR,LOrExpK,$1);
		SetNode_ASTType($$,AT_Condition);
		insert_ast_node($$,$3);
	}
	;

ConstExp :
	AddExp {
		//printf("%d : ConstExp -> AddExp\n", ++nCount);
		$$ = $1;
	}
	;

FuncDecl :
	BType T_ID '(' ')' ';' {
		//printf("%d : FuncDef -> BType Ident \"(\" \")\" \";\"\n", ++nCount);
		$$ = $2;
		SetOriginalType($$, FuncDeclK);
		SetNode_ASTType($$, AT_FuncDecl);
		SetBasicType($$, (ExpType)$1);

		$2 = new ast_node(FuncFParamsK);
		SetNode_ASTType($2, AT_FuncFParams);
		insert_ast_node($$,$2);
	}
	| BType T_ID '(' FuncFParams ')' ';' {
		//printf("%d : FuncDef -> BType Ident \"(\" FuncFParams \")\" \";\"\n", ++nCount);
		$$ = $2;
		SetOriginalType($$, FuncDeclK);
		SetNode_ASTType($$, AT_FuncDecl);
		SetBasicType($$, (ExpType)$1);

		insert_ast_node($$,$4);
	}
	| T_VOID T_ID '(' ')' ';' {
		//printf("%d : FuncDef -> \"void\" Ident \"(\" \")\" \";\"\n", ++nCount);
		$$ = $2;
		SetOriginalType($$, FuncDeclK);
		SetNode_ASTType($$, AT_FuncDecl);
		SetBasicType($$, (ExpType)Void);

		$2 = new ast_node(FuncFParamsK);
		SetNode_ASTType($2, AT_FuncFParams);
		insert_ast_node($$,$2);
	}
	| T_VOID T_ID '(' FuncFParams ')' ';' {
		//printf("%d : FuncDef -> \"void\" Ident \"(\" FuncFParams \")\" \";\"\n", ++nCount);
		$$ = $2;
		SetOriginalType($$, FuncDeclK);
		SetNode_ASTType($$, AT_FuncDecl);
		SetBasicType($$, (ExpType)Void);

		insert_ast_node($$,$4);
	}
    ;





%%

// 语法识别错误要调用函数的定义
void yyerror(char * msg)
{
    printf("Line %d here : %s\n", yylineno, msg);
}
