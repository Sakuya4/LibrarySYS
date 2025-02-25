#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <ctype.h>

/* 情況模擬大學圖書館 */
// 仍有不同之處

/* 視情況調整 */
#define MAXLEN 32
#define BOOKNAME 256
#define MAXID 10  //9碼員編or學號，員編和學號應不同長度, 應有更好的設計法
#define TABLESIZE 128
#define ISBN 13 //如果有ISSN or other 會另外寫
// #define ISSN 13
#define PHONE 10
#define MAXBOOK 100 //可借的書

// Reader management use linked list
// Reader 不限師長 or 學生
typedef struct Reader
{
    char Reader_ID[MAXID];
    char Reader_Name[MAXLEN];
    char phone[PHONE];
    struct Reader *next; //指向下一個Reader
} Reader;

/* Create Reader */
Reader* Create_Reader(char reader_ID[], const char* name, const char* contact)
{
    Reader* new_Reader = (Reader*)malloc(sizeof(Reader));
    if(new_Reader == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    strncpy(new_Reader -> Reader_ID, reader_ID, MAXID - 1);
    new_Reader->Reader_ID[MAXID - 1] = '\0';
    strncpy(new_Reader -> Reader_Name, name, MAXLEN - 1);
    new_Reader->Reader_Name[MAXLEN - 1] = '\0';
    strncpy(new_Reader -> phone, contact, PHONE - 1);
    new_Reader->phone[PHONE - 1] = '\0';
    new_Reader -> next = NULL;
    return new_Reader;
}

//解決memory leak
void Free_Reader_List(Reader* head)
{
    Reader* temp;
    while(head != NULL)
    {
        temp = head;
        head = head -> next;
        free(temp);
    }
}

/* Insert Reader to list */
void Insert_Reader(Reader** head, char reader_ID[], const char* name, const char* contact)
{
    Reader* new_Reader = Create_Reader(reader_ID, name, contact);
    // New reader insert 到 linked list 的head
    new_Reader -> next = *head; //New reader的next指向current的head
    *head = new_Reader;         //更新head為New reader
}

/* search user from list */
/* for time complexity maybe can design hash or high level sort + binary search */
Reader* Search_Reader(Reader* head, char reader_ID[])
{
    Reader* current = head;
    while(current != NULL)
    {
        if(strcmp(current -> Reader_ID, reader_ID) == 0) return current;
        current = current -> next;
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
    int probe_count = 0;
    while(BookTable[index] != NULL && probe_count < TABLESIZE) //if ISBN存在，更新數量
    {
        if(strcmp(BookTable[index]->Book_ISBN, new_book->Book_ISBN) == 0)
        {
            BookTable[index]->Quantity += new_book->Quantity;
            free(new_book);
            return;
        }
        index = (index + 1) % TABLESIZE;
        probe_count++;
    }
    if(probe_count == TABLESIZE)
    {
        fprintf(stderr, "Hash table is full\n");
        free(new_book);
        return;
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

void Free_Book_Table()
{
    for(int i = 0; i < TABLESIZE; i++)
    {
        Book* current = BookTable[i];
        while(current != NULL)
        {
            Book* temp = current;
            current = current->next;
            free(temp);
        }
        BookTable[i] = NULL;
    }
}

/* Search Book */

Book* Search_book(char* isbn)
{
    unsigned int index = hash(isbn);
    int probe_count = 0;
    
    while(BookTable[index] != NULL && probe_count < TABLESIZE)
    {
        if(strcmp(BookTable[index]->Book_ISBN, isbn) == 0)
        {
            return BookTable[index];
        }
        index = (index + 1) % TABLESIZE;
        probe_count++;
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
    if(node == NULL) return 0;
    return height(node->left) - height(node->right); //絕對值，高度不差2
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


void Free_AVL(AVLTree* root)
{
    if(root == NULL) return;
    Free_AVL(root->left);
    Free_AVL(root->right);
    if(root->book)
    {
        free(root->book);
    }
    free(root);
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
    Book* books[MAXBOOK];
    int book_count;
    struct BorrowRecord* next;
} BorrowRecord;

BorrowRecord* Create_borrow_record(Reader* reader_id, Book* book)
{
    BorrowRecord* new_record = malloc(sizeof(BorrowRecord));
    if(!new_record)
    {
        perror("Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    new_record->readerID = reader_id;
    memset(new_record->books, 0, sizeof(new_record->books));
    
    if(book != NULL) new_record->books[0] = book;
    new_record->book_count = book ? 1:0 ;
    new_record->next = NULL;
    return new_record;
}

void Free_Borrow_Record(BorrowRecord* head)
{
    BorrowRecord* temp;
    while(head != NULL)
    {
        temp = head;
        head = head->next;
        while(head != NULL)
        {
            temp = head;
            head = head->next;
            free(temp);
        }
    }
}



void Insert_borrow_record(BorrowRecord** head, Reader* reader_id, Book* book)
{
    BorrowRecord* new_record = Create_borrow_record(reader_id, book);
    new_record->next = *head;
    *head = new_record;
}


void ShowMenu() {
    printf("\n=========== University Library System ===========\n");
    printf("1. Add Reader\n");
    printf("2. Search Reader\n");
    printf("3. Add Book\n");
    printf("4. Search Book\n");
    printf("5. Borrow Book\n");
    printf("6. Display All Readers\n");
    printf("7. Display All Books\n");
    printf("8. Exit\n");
    printf("===============================================\n");
    printf("Please select an option: ");
}

// Main program: handling user commands
int main() {
    Reader* reader_list = NULL;   // Reader linked list
    BorrowRecord* borrow_list = NULL; // Borrow records
    Book* BookTable[TABLESIZE] = {0}; // Book hash table

    int choice;

    // Continuously display the menu until the user selects exit
    while (1) {
        ShowMenu();
        scanf("%d", &choice); // Get user choice

        // Execute corresponding actions based on user choice
        switch (choice) {
            case 1: {
                // Add Reader
                char reader_ID[MAXID], name[MAXLEN], phone[PHONE];
                printf("Enter Reader ID: ");
                scanf("%s", reader_ID);
                printf("Enter Reader Name: ");
                scanf("%s", name);
                printf("Enter Phone Number: ");
                scanf("%s", phone);

                Insert_Reader(&reader_list, reader_ID, name, phone);
                printf("Reader %s has been added.\n", name);
                break;
            }
            case 2: {
                // Search Reader
                char reader_ID[MAXID];
                printf("Enter Reader ID: ");
                scanf("%s", reader_ID);

                Reader* found_reader = Search_Reader(reader_list, reader_ID);
                if (found_reader) {
                    printf("Reader Info: ID=%s, Name=%s, Phone=%s\n", found_reader->Reader_ID, found_reader->Reader_Name, found_reader->phone);
                } else {
                    printf("Reader not found.\n");
                }
                break;
            }
            case 3: {
                // Add Book
                char isbn[ISBN], name[MAXBOOK], author[MAXLEN];
                int quantity;
                printf("Enter Book ISBN: ");
                scanf("%s", isbn);
                printf("Enter Book Name: ");
                scanf("%s", name);
                printf("Enter Author: ");
                scanf("%s", author);
                printf("Enter Quantity: ");
                scanf("%d", &quantity);

                Book* new_book = (Book*)malloc(sizeof(Book));
                strncpy(new_book->Book_ISBN, isbn, ISBN - 1);
                new_book->Book_ISBN[ISBN - 1] = '\0';
                strncpy(new_book->Book_Name, name, MAXBOOK - 1);
                new_book->Book_Name[MAXBOOK - 1] = '\0';
                strncpy(new_book->Author, author, MAXLEN - 1);
                new_book->Author[MAXLEN - 1] = '\0';
                new_book->Quantity = quantity;
                new_book->Borrowed = 0;

                Insert_Book_Linear_Probing(new_book);
                printf("Book %s has been added.\n", name);
                break;
            }
            case 4: {
                // Search Book
                char isbn[ISBN];
                printf("Enter Book ISBN: ");
                scanf("%s", isbn);

                Book* found_book = Search_book(isbn);
                if (found_book) {
                    printf("Book Info: ISBN=%s, Name=%s, Author=%s, Available Quantity=%d\n", found_book->Book_ISBN, found_book->Book_Name, found_book->Author, found_book->Quantity);
                } else {
                    printf("Book not found.\n");
                }
                break;
            }
            case 5: {
                // Borrow Book
                char reader_ID[MAXID], isbn[ISBN];
                printf("Enter Reader ID: ");
                scanf("%s", reader_ID);
                printf("Enter Book ISBN: ");
                scanf("%s", isbn);

                Reader* found_reader = Search_Reader(reader_list, reader_ID);
                Book* found_book = Search_book(isbn);
                if (found_reader && found_book) {
                    if (found_book->Quantity > found_book->Borrowed) {
                        Insert_borrow_record(&borrow_list, found_reader, found_book);
                        found_book->Borrowed++;
                        printf("Successfully borrowed: %s\n", found_book->Book_Name);
                    } else {
                        printf("Book out of stock, cannot borrow.\n");
                    }
                } else {
                    printf("Reader or Book not found, cannot borrow.\n");
                }
                break;
            }
            case 6: {
                // Display All Readers
                Reader* current = reader_list;
                while (current != NULL) {
                    printf("Reader Info: ID=%s, Name=%s, Phone=%s\n", current->Reader_ID, current->Reader_Name, current->phone);
                    current = current->next;
                }
                break;
            }
            case 7: {
                // Display All Books
                for (int i = 0; i < TABLESIZE; i++) {
                    Book* current = BookTable[i];
                    while (current != NULL) {
                        printf("Book Info: ISBN=%s, Name=%s, Author=%s, Available Quantity=%d\n", current->Book_ISBN, current->Book_Name, current->Author, current->Quantity);
                        current = current->next;
                    }
                }
                break;
            }
            case 8:
                // Exit
                printf("Exiting the system.\n");
                // Free resources
                Free_Reader_List(reader_list);
                Free_Book_Table();
                Free_Borrow_Record(borrow_list);
                return 0;
            default:
                printf("Invalid choice, please try again.\n");
        }
    }

    return 0;
}
