#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <ctype.h>

/* 情況模擬大學圖書館 */
// 仍有不同之處

/* 視情況調整 */
#define MAXLEN 32
#define BOOKNAME 256
#define MAXID 9  //員編和學號應不同長度, 應有更好的設計法
#define TABLESIZE 128
#define ISBN 13 //如果有ISSN or other 會另外寫
// #define ISSN 13
#define PHONE 10
#define MAXBOOK 100 //可借的書

// Reader management use linked list
// Reader 不限師長 or 學生
typedef struct Reader
{
    int Reader_ID[MAXID];
    char Reader_Name[MAXLEN];
    char phone[PHONE];
    struct Reader *next; //指向下一個Reader
} Reader;

/* Create Reader */
Reader* Create_Reader(int reader_ID[], const char* name, const char* contact)
{
    Reader* new_Reader = (Reader*)malloc(sizeof(Reader));
    // new_Reader->Reader_ID = Reader_ID;
    for(int i = 0; i < MAXID; i++)
    {
        new_Reader->Reader_ID[i] = reader_ID[i];
    }
    strcpy(new_Reader->Reader_Name, name);
    strcpy(new_Reader->phone, contact);
    new_Reader->next = NULL;
    return new_Reader;
}

/* Insert Reader to list */
void Insert_Reader(Reader** head, int reader_ID[], const char* name, const char* contact)
{
    Reader* new_Reader = Create_Reader(reader_ID, name, contact);
    // New reader insert 到 linked list 的head
    new_Reader -> next = *head; //New reader的next指向current的head
    *head = new_Reader;         //更新head為New reader
}

/* search user from list */
/* for time complexity maybe can design hash or high level sort + binary search */
Reader* Search_Reader(Reader* head, int reader_ID[])
{
    Reader* current = head;
    while(current != NULL)
    {
        int match = 1;
        for(int i = 0; i < MAXID; i++)
        {
            if(current->Reader_ID[i] != reader_ID[i])
            {
                match = 0;
                break;  //不同, match設0
            }
        }
        if(match) return current;
        current = current->next;
    }
    return NULL; // or return "Not found"
}




/* book management block*/
/* this time I use hash table*/

typedef struct Book
{
    char Book_ISBN[ISBN];
    char Book_Name[BOOKNAME];
    char Author[MAXLEN];
    int Quantity;
    int Borrowed;
    struct Book *next; //指向下一個Book
} Book;

Book* BookTable[TABLESIZE];

//Hash function
unsigned int hash(char* isbn)
{
    unsigned int hash = 0;
    while(*isbn){hash = (hash << 4) + *isbn++;}
    return hash % TABLESIZE;
}

/*Insert, we use "Open Addressing" to solve "collision", three method */


//Normal Insert

void Insert_Book(Book* new_book)
{
    unsigned int index = hash(new_book->Book_ISBN);
    new_book->next = BookTable[index];
    BookTable[index] = new_book;
}


//linear probing
void Insert_Book_Linear_Probing(Book* new_book)
{
    unsigned int index = hash(new_book->Book_ISBN);
    while(BookTable[index] != NULL) //if ISBN存在，更新數量
    {
        if(strcmp(BookTable[index]->Book_ISBN, new_book->Book_ISBN) == 0)
        {
            BookTable[index]->Quantity += new_book->Quantity;
            free(new_book);
            return;
        }
        index = (index + 1) % TABLESIZE;
    }
    BookTable[index] = new_book;
}

//quadratic probing
void Insert_Book_Quadratic_Probing(Book* new_book)
{
    unsigned int index = hash(new_book->Book_ISBN);
    int i = 0;//探測次數
    while(BookTable[index] != NULL)
    {
        if(strcmp(BookTable[index]->Book_ISBN, new_book->Book_ISBN) == 0) //if ISBN存在，更新數量
        {
            BookTable[index]->Quantity += new_book->Quantity;
            free(new_book);
            return;
        }
        i++;
        index = (index + i*i) % TABLESIZE; //Quadratic Probing之公式
    }
    BookTable[index] = new_book;
}

// double probing
unsigned int hash2(char* isbn)
{
    unsigned int hash = 0;
    while(*isbn){hash = (hash << 4) + *isbn++;}
    return 1 + (hash % (TABLESIZE - 1));
}


void Insert_book_Double_Probing(Book* new_book)
{
    unsigned int index = hash(new_book->Book_ISBN);
    unsigned int step = hash2(new_book->Book_ISBN);

    while(BookTable[index] != NULL)
    {
        if(strcmp(BookTable[index]->Book_ISBN, new_book->Book_ISBN) == 0)
        {
            BookTable[index]->Quantity += new_book->Quantity;
            free(new_book);
            return;
        }
        index = (index + step) % TABLESIZE;
    }
    BookTable[index] = new_book;
}

/* Search Book */

Book* Search_book(char* isbn)
{
    unsigned int index = hash(isbn);
    Book* current = BookTable[index];
    while(current != NULL)
    {
        if(strcmp(current->Book_ISBN, isbn) == 0) return current;
        current = current->next;
    }
    return NULL;
}

/* 書籍量非常大，考慮到性能，我想使用高等樹來提升效率，以AVL tree來處理 */
typedef struct AVLTree
{
    Book* book; //存書籍的指針
    struct AVLTree* left;
    struct AVLTree* right;
    int height; //用來計算樹的高度以進行Rotation
} AVLTree;

int height(AVLTree* node)
{
    if(node == NULL) return 0;
    return node->height;
}


int max(int a, int b)
{
    return (a > b) ? a : b;
}

void UpdateHeight(AVLTree* node)
{
    if(node == NULL) return;
    
    node->height = 1 + max(height(node->left), height(node->right));
}

int BalancedPoints(AVLTree* node)
{
    return height(node->left) - height(node->right);
}

//right rotation
AVLTree* rightRotate(AVLTree* y)
{
    AVLTree* x = y->left;
    AVLTree* T2 = x->right;

    x->right = y;
    y->left = T2;

    UpdateHeight(y);
    UpdateHeight(x);

    return x;
}
//left rotation
AVLTree* leftRotate(AVLTree* x)
{
    AVLTree* y = x->right;
    AVLTree* T2 = y->left;

    y->left = x;
    x->right = T2;

    UpdateHeight(x);
    UpdateHeight(y);

    return y;
}

//Insert book to AVL tree
AVLTree* insertAVL(AVLTree* node, Book* new_book)
{
    if (node == NULL) {
        AVLTree* newNode = malloc(sizeof(AVLTree));
        newNode->book = new_book;
        newNode->left = newNode->right = NULL;
        newNode->height = 1; // 新節點高度為1
        return newNode;
    }

    // 根據 ISBN 插入
    if(strcmp(new_book->Book_ISBN, node->book->Book_ISBN) < 0){node->left = insertAVL(node->left, new_book);}
    else if(strcmp(new_book->Book_ISBN, node->book->Book_ISBN) > 0){node->right = insertAVL(node->right, new_book);}
    else return node; // 重複 ISBN，不插入

    UpdateHeight(node);
    int balance = BalancedPoints(node);
    //LL
    if(balance > 1 && strcmp(new_book->Book_ISBN, node->left->book->Book_ISBN) < 0) return rightRotate(node);
    //RR
    if (balance < -1 && strcmp(new_book->Book_ISBN, node->right->book->Book_ISBN) > 0) return leftRotate(node);
    //LR
    if(balance > 1 && strcmp(new_book->Book_ISBN, node->left->book->Book_ISBN) > 0)
    {
        node->left = leftRotate(node->left);
        return rightRotate(node);
    }
    //RL
    if(balance < -1 && strcmp(new_book->Book_ISBN, node->right->book->Book_ISBN) < 0)
    {
        node->right = rightRotate(node->right);
        return leftRotate(node);
    }

    return node; // 返回未改變的節點指針
}

// 查找書籍在 AVL 樹中的位置
Book* Search_Book_AVL(AVLTree* root, char* isbn)
{
    if (root == NULL || strcmp(root->book->Book_ISBN, isbn) == 0){return root ? root->book : NULL;}
    if (strcmp(isbn, root->book->Book_ISBN) < 0){return Search_Book_AVL(root->left, isbn);}
    
    return Search_Book_AVL(root->right, isbn);
}


/* 借書 */
typedef struct BorrowRecord
{
    Reader* readerID;
    Book* isbn[ISBN];
    struct BorrowRecord* next;
} BorrowRecord;

BorrowRecord* Create_borrow_record(Reader* reader_id, Book* book)
{
    BorrowRecord* new_record = (BorrowRecord*)malloc(sizeof(BorrowRecord));
    if(new_record == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    new_record->readerID = reader_id;
    new_record->isbn[0] = book;
    new_record->next = NULL;
    return new_record;
}

void Insert_borrow_record(BorrowRecord** head, Reader* reader_id, Book* book)
{
    BorrowRecord* new_record = Create_borrow_record(reader_id, book);
    new_record->next = *head;
    *head = new_record;
}

int main() {

    return 0;
}
