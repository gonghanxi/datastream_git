#ifndef SAFELIST_H
#define SAFELIST_H

// 定义一个模板结构体 Node，表示链表中的一个节点
template<typename T>
struct Node {
    T value;          // 节点中存储的数据
    Node* prev;       // 指向前一个节点的指针
    Node* next;       // 指向后一个节点的指针

    // Node 的构造函数
    // v: 初始化节点的值
    Node():
          prev(nullptr), // 前驱指针初始化为空
          next(nullptr)  // 后继指针初始化为空
    {}
};
// 定义一个模板类 SafeList，表示一个支持“安全遍历 + 修改”的双向链表
template<typename T>
class SafeList {
private:

public:
    // SafeList 默认构造函数
    // 初始化头尾指针为空
    SafeList() : headNode(nullptr), tail(nullptr) {}

    // SafeList 析构函数
    // 在对象销毁时释放所有节点内存
    ~SafeList() { clear(); }


    bool isTail(Node<T>* n)
    {
        return  n==tail;
    }

    // 向链表尾部插入一个新元素
    void push_back(Node<T>* n) {
        // 为新值创建一个新节点
//        Node<T>* n = new Node<T>(v);

        // 如果链表为空，头尾指针都指向新节点
        if (!headNode) headNode = tail = n;
        else {
            // 原尾节点的 next 指向新节点
            tail->next = n;

            // 新节点的 prev 指向原尾节点
            n->prev = tail;

            // 更新尾指针
            tail = n;
        }
    }

    // 清空整个链表，释放所有节点
    void clear() {
        // 从头节点开始遍历
        Node<T>* p = headNode;

        // 逐个释放节点
        while (p) {
            Node<T>* t = p;   // 保存当前节点
            p = p->next;      // 移动到下一个节点
            delete t;         // 释放当前节点内存
        }

        // 重置头尾指针
        headNode = tail = nullptr;
    }

//    bool  isTail(Node<T>*cur)
//    {
//        return cur == tail;
//    }

    // ===== 核心接口 =====

    // 获取链表第一个节点（用于 while-next 遍历起点）
    Node<T>* first() { return headNode; }

    // 删除当前节点（安全：不会破坏遍历）
    // cur: 当前节点引用（可能被修改）
    // next: 用于保存“下一个节点”，防止遍历失效
     Node<T>*  erase(Node<T>*cur) {
        // 保存即将删除的节点
        Node<T>* toDel = cur;

        // 如果当前节点不是头节点
        if (cur->prev)  //头节点的prev为空
            cur->prev->next = cur->next;  // 前驱指向后继
        else
            headNode = cur->next;        // 更新头指针

        // 如果当前节点不是尾节点
        if (cur->next) //尾节点的next为空
            cur->next->prev = cur->prev; // 后继指向前驱
        else
            tail = cur->prev;            // 更新尾指针

        // 提前保存下一个节点（关键！）
        Node<T>* next = cur->next;

        // 释放当前节点
        delete cur;

        // 将 cur 移动到 next，保证遍历继续
        return next;
    }

    // 在当前节点后插入一个新节点
     void insert_after(Node<T>* cur, Node<T>* n ) {
        // 创建新节点
//        Node<T>* n = new Node<T>(v);

        // 新节点的前驱是当前节点
        n->prev = cur;

        // 新节点的后继是当前节点的后继
        n->next = cur->next;

        // 如果当前节点不是尾节点
        if (cur->next)
            cur->next->prev = n; // 原后继的前驱指向新节点
        else
            tail = n;            // 更新尾指针

        // 当前节点的 next 指向新节点
        cur->next = n;
    }

    // 在当前节点前插入一个新节点
     void insert_before(Node<T>* cur, Node<T>* n) {
        // 创建新节点
//        Node<T>* n = new Node<T>(v);

        // 新节点的后继是当前节点
        n->next = cur;

        // 新节点的前驱是当前节点的前驱
        n->prev = cur->prev;

        // 如果当前节点不是头节点
        if (cur->prev)
            cur->prev->next = n; // 原前驱的后继指向新节点
        else
            headNode = n;            // 更新头指针

        // 当前节点的前驱指向新节点
        cur->prev = n;
    }

private:
    Node<T>* headNode; // 指向链表头节点
    Node<T>* tail; // 指向链表尾节点
};
#endif // SAFELIST_H
