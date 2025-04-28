#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ---- GLOBAL SABİTLER ----
#define MAX_USERS 999
#define MAX_FRIENDS 20

// ---- YAPILAR ve PROTOTİPLER ----

// Kullanıcı listesi ve arkadaşlıklar için
typedef struct User {
    int id;
    struct User* friends[MAX_FRIENDS];
    int friend_count;
} User;

// Kullanıcı listesi
typedef struct UserList {
    User* users[MAX_USERS];
    int count;
} UserList;

// DFS için Stack
typedef struct StackNode {
    User* user;
    int depth;
    struct StackNode* next;
} StackNode;

void push(StackNode** top, User* user, int depth);
StackNode* pop(StackNode** top);
int isStackEmpty(StackNode* top);

// Kırmızı-Siyah Ağaç için renkler ve düğüm
typedef enum { RED, BLACK } Color;
typedef struct RBTreeNode {
    int user_id;
    User* user_ptr;
    Color color;
    struct RBTreeNode *left, *right, *parent;
} RBTreeNode;

RBTreeNode* rb_insert(RBTreeNode* root, int user_id, User* user_ptr);
RBTreeNode* rb_search(RBTreeNode* root, int user_id);
void inorder_rb(RBTreeNode* root);

// Fonksiyon prototipleri
void load_dataset(const char* filename, UserList* user_list);
User* find_user(UserList* list, int id);
void add_friend(User* u1, User* u2);
void print_menu();
void dfs_at_distance(User* start, int max_depth);
void print_tree_levels(User* start, int max_depth);
void find_common_friends(User* u1, User* u2);
void find_communities_and_isolated(UserList* list);
void calculate_influence(User* user);
void free_users(UserList* list);
void free_rb_tree(RBTreeNode* root);

// ---- ANA PROGRAM ----
int main() {
    UserList user_list = { .count = 0 };
    RBTreeNode* rb_root = NULL;
    int choice, uid, uid2, depth;
    char filename[100] = "veriseti.txt";

    load_dataset(filename, &user_list); // Kullanıcı ve arkadaşlık verilerini yükle

    // Kullanıcıları kırmızı-siyah ağaca ekle
    for (int i = 0; i < user_list.count; i++) {
        rb_root = rb_insert(rb_root, user_list.users[i]->id, user_list.users[i]);
    }

    // Ana menü döngüsü
    while (1) {
        print_menu();
        printf("Seçiminiz: ");
        scanf("%d", &choice);

        if (choice == 1) {
            // Arkadaşlık ağacı gösterimi (BFS)
            printf("Ağacın kök kullanıcı ID'sini giriniz: ");
            scanf("%d", &uid);
            User* u = find_user(&user_list, uid);
            if (u) {
                printf("%d kullanıcı ID'li kullanıcının ağacı:\n", uid);
                print_tree_levels(u, 5); // 5 veya ihtiyaca göre derinlik
            } else {
                printf("Kullanıcı bulunamadı!\n");
            }
        }else if (choice == 2) {
            // DFS ile belirli mesafedeki arkadaşları bulma
            printf("Kullanıcı ID: ");
            scanf("%d", &uid);
            printf("Mesafe (depth): ");
            scanf("%d", &depth);
            User* u = find_user(&user_list, uid);
            if (u) {
                dfs_at_distance(u, depth);
            } else {
                printf("Kullanıcı bulunamadı!\n");
            }
        } else if (choice == 3) {
            // İki kullanıcı arasında ortak arkadaş analizi
            printf("İlk kullanıcı ID: ");
            scanf("%d", &uid);
            printf("İkinci kullanıcı ID: ");
            scanf("%d", &uid2);
            User* u1 = find_user(&user_list, uid);
            User* u2 = find_user(&user_list, uid2);
            if (u1 && u2) {
                find_common_friends(u1, u2);
            } else {
                printf("Kullanıcı(lar) bulunamadı!\n");
            }
        } else if (choice == 4) {
            // Topluluk (community) tespiti
            printf("Bağlı topluluklar (community) tespiti:\n");
            find_communities_and_isolated(&user_list);
        } else if (choice == 5) {
            // Kullanıcının etki alanı (reach) analizi
            printf("Kullanıcı ID (etki alanı için): ");
            scanf("%d", &uid);
            User* u = find_user(&user_list, uid);
            if (u) {
                calculate_influence(u);
            } else {
                printf("Kullanıcı bulunamadı!\n");
            }
        } else if (choice == 6) {
            // Kırmızı-Siyah Ağaç ile kullanıcı arama
            printf("Aranacak kullanıcı ID: ");
            scanf("%d", &uid);
            RBTreeNode* res = rb_search(rb_root, uid);
            printf(res && res->user_ptr ? "Kullanıcı bulundu: %d\n" : "Kullanıcı bulunamadı!\n", uid);
        } else if (choice == 7) {
            // Kırmızı-Siyah Ağaç sıralı liste (inorder dolaşım)
            printf("Kullanıcılar ID sırasıyla (Kırmızı-Siyah Ağacı, Inorder):\n");
            inorder_rb(rb_root);
            printf("\n");
        } else if (choice == 0) {
            // Çıkış
            break;
        } else {
            printf("Geçersiz seçim!\n");
        }
    }

    free_users(&user_list);     // Bellek temizliği
    free_rb_tree(rb_root);      // Bellek temizliği
    return 0;
}

// ---- FONKSİYONLAR ----


// Kullanıcı listesinden ID ile kullanıcıyı bulur ve döndürür.
// Eğer kullanıcı yoksa NULL döner.
User* find_user(UserList* list, int id) {
    for (int i = 0; i < list->count; i++)
        if (list->users[i]->id == id) return list->users[i];
    return NULL;
}

// İki kullanıcı arasında çift yönlü arkadaşlık ilişkisi kurar.
// Her iki kullanıcının arkadaş listesine de diğerini ekler.
void add_friend(User* u1, User* u2) {
    // u1'in arkadaşları arasında u2 var mı? zaten arkadaşlarsa tekrar ekleme
    for (int i = 0; i < u1->friend_count; i++)
        if (u1->friends[i] == u2) return;
    // u2'nin arkadaşları arasında u1 var mı?
    for (int i = 0; i < u2->friend_count; i++)
        if (u2->friends[i] == u1) return;

    if (u1->friend_count < MAX_FRIENDS && u2->friend_count < MAX_FRIENDS) {
        u1->friends[u1->friend_count++] = u2;
        u2->friends[u2->friend_count++] = u1;
    }
}

// veriseti.txt dosyasını okuyarak kullanıcıları ve arkadaşlık ilişkilerini belleğe yükler.
// Her "USER" satırında yeni bir kullanıcı oluşturur, "FRIEND" satırında ise iki kullanıcı arasında arkadaşlık bağlantısı kurar.
void load_dataset(const char* filename, UserList* user_list) {
    FILE* fp = fopen(filename, "r");
    if (!fp) { printf("veriseti.txt bulunamadı\n"); exit(1);}
    char line[100], type[20];
    int id1, id2;
    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "USER %d", &id1) == 1) {
            User* u = (User*)malloc(sizeof(User));
            u->id = id1; u->friend_count = 0;
            user_list->users[user_list->count++] = u;
        } else if (sscanf(line, "FRIEND %d %d", &id1, &id2) == 2) {
            User* u1 = find_user(user_list, id1);
            User* u2 = find_user(user_list, id2);
            if (u1 && u2) add_friend(u1, u2);
        }
    }
    fclose(fp);
}

// Menü ekrana yazdırılır
void print_menu() {
    printf("\n=== MENÜ ===\n");
    printf("1. İlişki ağacı oluşturma (kullanıcılar için)\n");
    printf("2. Belirli mesafedeki arkadaşları bul (DFS)\n");
    printf("3. Ortak arkadaş analizi\n");
    printf("4. Topluluk (community) tespiti\n");
    printf("5. Etki alanı (influence) analizi\n");
    printf("6. Kırmızı-Siyah Ağacı ile kullanıcı arama\n");
    printf("7. Kırmızı-Siyah Ağacı sıralı listele (Inorder)\n");
    printf("0. Çıkış\n");
}

// DFS ile ağacın derinliklerine iner ve belli mesafedeki arkadaşları bulur
// Derinlik öncelikli arama (DFS) algoritmasıyla verilen kullanıcıdan başlayarak
// en fazla max_depth kadar derinliğe sahip arkadaşları ekrana yazar.
// Ziyaret edilen kullanıcılar tekrar ziyaret edilmez.
void dfs_at_distance(User* start, int distance) {
    int visited[MAX_USERS] = {0};
    StackNode* stack = NULL;
    int found = 0;
    push(&stack, start, 0);

    printf("%d mesafesindeki arkadaş(lar):\n", distance);
    while (!isStackEmpty(stack)) {
        StackNode* node = pop(&stack);
        User* u = node->user;
        int d = node->depth;
        free(node);

        if (visited[u->id]) continue;
        visited[u->id] = 1;

        if (d == distance && u != start) {
            printf("ID: %d\n", u->id);
            found = 1;
        }

        if (d < distance) {
            for (int i = 0; i < u->friend_count; i++) {
                if (!visited[u->friends[i]->id])
                    push(&stack, u->friends[i], d + 1);
            }
        }
    }
    if (!found)
        printf("Bu mesafede arkadaş yok.\n");
}

// BFS ile her seviyedeki kullanıcıları bulur ve yazdırır
void print_tree_levels(User* start, int max_depth) {
    int visited[MAX_USERS] = {0};
    int level[MAX_USERS] = {0};
    User* queue[MAX_USERS];
    int front = 0, rear = 0;

    queue[rear++] = start;
    visited[start->id] = 1;
    level[start->id] = 0;

    // BFS ile seviyeleri bul
    while (front < rear) {
        User* u = queue[front++];
        int curr_level = level[u->id];
        if (curr_level >= max_depth) continue;
        for (int i = 0; i < u->friend_count; i++) {
            int fid = u->friends[i]->id;
            if (!visited[fid]) {
                queue[rear++] = u->friends[i];
                visited[fid] = 1;
                level[fid] = curr_level + 1;
            }
        }
    }

    // SADECE ARKADAŞ OLAN MESAFELERİ YAZDIR!
    for (int d = 1; d <= max_depth; d++) {
        int found = 0;
        for (int i = 0; i < MAX_USERS; i++) {
            if (visited[i] && level[i] == d) {
                if (!found) {
                    printf("%d mesafesindeki arkadaş(lar):\n", d);
                }
                printf("ID: %d\n", i);
                found = 1;
            }
        }
        // found == 0 ise hiçbir şey yazma, o mesafeyi atla
    }
}

// İki kullanıcı arasındaki ortak arkadaşları bulur ve ekrana yazar.
void find_common_friends(User* u1, User* u2) {
    int is_friend[MAX_USERS] = {0};
    for (int i = 0; i < u1->friend_count; i++)
        is_friend[u1->friends[i]->id] = 1;

    printf("Ortak arkadaşlar: ");
    int found = 0;
    for (int j = 0; j < u2->friend_count; j++) {
        if (is_friend[u2->friends[j]->id]) {
            printf("%d ", u2->friends[j]->id);
            found = 1;
        }
    }
    if (!found) printf("yok");
    printf("\n");
}

// Topluluk tespiti (Basit bağlı bileşenler)
// DFS ile toplulukları (bağlı bileşenleri) işaretler
void dfs_mark(User* u, int* visited) {
    visited[u->id] = 1;
    for (int i = 0; i < u->friend_count; i++) {
        if (!visited[u->friends[i]->id])
            dfs_mark(u->friends[i], visited);
    }
}

// DFS ile topluluk üyelerini bir diziye toplar
void dfs_collect(User* u, int* visited, int* members, int* member_count) {
    visited[u->id] = 1;
    members[(*member_count)++] = u->id;
    for (int i = 0; i < u->friend_count; i++) {
        if (!visited[u->friends[i]->id])
            dfs_collect(u->friends[i], visited, members, member_count);
    }
}

// Tüm toplulukları ve izole kullanıcıları yazdırır
void find_communities_and_isolated(UserList* list) {
    printf("\n");
    int visited[MAX_USERS] = {0}, comm = 0;
    for (int i = 0; i < list->count; i++) {
        if (!visited[list->users[i]->id]) {
            int members[MAX_USERS];
            int member_count = 0;
            dfs_collect(list->users[i], visited, members, &member_count);
            if (member_count == 1 && list->users[i]->friend_count == 0) {
                // İzole kullanıcı, ayrı yazılır
                printf("İzole kullanıcı: %d\n", list->users[i]->id);
            } else {
                printf("Topluluk %d (Üyeler: ", ++comm);
                for (int j = 0; j < member_count; j++)
                    printf("%d%s", members[j], j == member_count-1 ? "" : ", ");
                printf(")\n");
            }
        }
    }
}

// Etki alanı (reach) analizi
// Bir kullanıcının sosyal ağdaki etki alanını (reach) hesaplar.
// Kullanıcının ulaşabileceği toplam kişi sayısını bulur ve ekrana yazar.
void calculate_influence(User* user) {
    int visited[MAX_USERS] = {0}, count = 0;
    StackNode* stack = NULL;
    push(&stack, user, 0);
    printf("%d numaralı kullanıcının etki alanı:\n", user->id);

    while (!isStackEmpty(stack)) {
        StackNode* node = pop(&stack);
        User* u = node->user;
        free(node);
        if (visited[u->id]) continue;
        visited[u->id] = 1;
        if (u != user) { // Kendisi hariç
            printf("ID: %d\n", u->id);
            count++;
        }
        for (int i = 0; i < u->friend_count; i++) {
            if (!visited[u->friends[i]->id])
                push(&stack, u->friends[i], 0);
        }
    }
    printf("Toplam: %d kişi\n", count);
}
// Stack fonksiyonları

void push(StackNode** top, User* user, int depth) {
    StackNode* node = (StackNode*)malloc(sizeof(StackNode));
    node->user = user; node->depth = depth;
    node->next = *top; *top = node;
}
StackNode* pop(StackNode** top) {
    if (!*top) return NULL;
    StackNode* node = *top;
    *top = node->next;
    return node;
}
int isStackEmpty(StackNode* top) {
    return top == NULL;
}

// ---- Kırmızı-Siyah Ağaç (RBTree) Temel Fonksiyonları ----

// RBTree yardımcı fonksiyonları
RBTreeNode* create_rb_node(int user_id, User* user_ptr) {
    RBTreeNode* node = (RBTreeNode*)malloc(sizeof(RBTreeNode));
    node->user_id = user_id; node->user_ptr = user_ptr;
    node->color = RED; node->left = node->right = node->parent = NULL;
    return node;
}

// RBTree'de sola döndürme işlemi
RBTreeNode* rotate_left(RBTreeNode* root, RBTreeNode* x) {
    RBTreeNode* y = x->right;
    x->right = y->left;
    if (y->left) y->left->parent = x;
    y->parent = x->parent;
    if (!x->parent) root = y;
    else if (x == x->parent->left) x->parent->left = y;
    else x->parent->right = y;
    y->left = x; x->parent = y;
    return root;
}

// RBTree'de sağa döndürme işlemi
RBTreeNode* rotate_right(RBTreeNode* root, RBTreeNode* y) {
    RBTreeNode* x = y->left;
    y->left = x->right;
    if (x->right) x->right->parent = y;
    x->parent = y->parent;
    if (!y->parent) root = x;
    else if (y == y->parent->left) y->parent->left = x;
    else y->parent->right = x;
    x->right = y; y->parent = x;
    return root;
}

// RBTree'ye ekleme sonrası dengeleme işlemi
RBTreeNode* fix_insert(RBTreeNode* root, RBTreeNode* z) {
    while (z->parent && z->parent->color == RED) {
        RBTreeNode* gp = z->parent->parent;
        if (z->parent == gp->left) {
            RBTreeNode* y = gp->right;
            if (y && y->color == RED) {
                z->parent->color = BLACK; y->color = BLACK; gp->color = RED; z = gp;
            } else {
                if (z == z->parent->right) { z = z->parent; root = rotate_left(root, z);}
                z->parent->color = BLACK; gp->color = RED; root = rotate_right(root, gp);
            }
        } else {
            RBTreeNode* y = gp->left;
            if (y && y->color == RED) {
                z->parent->color = BLACK; y->color = BLACK; gp->color = RED; z = gp;
            } else {
                if (z == z->parent->left) { z = z->parent; root = rotate_right(root, z);}
                z->parent->color = BLACK; gp->color = RED; root = rotate_left(root, gp);
            }
        }
    }
    root->color = BLACK;
    return root;
}

// Kırmızı-siyah ağaç yapısına yeni bir kullanıcı ekler.
// Ağacın dengeli ve kırmızı-siyah kurallarına uygun kalmasını sağlar.
RBTreeNode* rb_insert(RBTreeNode* root, int user_id, User* user_ptr) {
    RBTreeNode *y = NULL, *x = root;
    while (x) {
        y = x;
        if (user_id < x->user_id) x = x->left;
        else if (user_id > x->user_id) x = x->right;
        else return root; // Aynı ID eklenmez
    }
    RBTreeNode* z = create_rb_node(user_id, user_ptr);
    z->parent = y;
    if (!y) root = z;
    else if (user_id < y->user_id) y->left = z;
    else y->right = z;
    return fix_insert(root, z);
}

// Kırmızı-siyah ağaçta verilen kullanıcı ID'sini arar.
// Bulursa ilgili düğümü, bulamazsa NULL döner.
RBTreeNode* rb_search(RBTreeNode* root, int user_id) {
    while (root) {
        if (user_id == root->user_id) return root;
        root = (user_id < root->user_id) ? root->left : root->right;
    }
    return NULL;
}

// RBTree'yi sıralı (inorder) dolaşır
void inorder_rb(RBTreeNode* root) {
    if (!root) return;
    inorder_rb(root->left);
    printf("%d ", root->user_id);
    inorder_rb(root->right);
}

// Kullanıcılar için ayrılan belleği serbest bırakır
void free_users(UserList* list) {
    for (int i = 0; i < list->count; i++)
        free(list->users[i]);
}

// Kırmızı-siyah ağaçtaki tüm düğümler için ayrılan belleği serbest bırakır.
void free_rb_tree(RBTreeNode* root) {
    if (!root) return;
    free_rb_tree(root->left);
    free_rb_tree(root->right);
    free(root);
}

