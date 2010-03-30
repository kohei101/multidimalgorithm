/*************************************************************************
 *
 * Copyright (c) 2010 Kohei Yoshida
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 ************************************************************************/

#ifndef __SEGMENTTREE_HPP__
#define __SEGMENTTREE_HPP__

#include "node.hpp"

#include <vector>
#include <list>
#include <iostream>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/ptr_container/ptr_map.hpp>

#if UNIT_TEST
#include <sstream>
#endif

namespace mdds {

template<typename _Key, typename _Data>
class segment_tree
{
public:
    typedef _Key        key_type;
    typedef _Data       data_type;
    typedef ::std::list<const data_type*> data_chain_type;

private:
    struct segment_data
    {
        key_type    begin_key;
        key_type    end_key;
        data_type*  pdata;

        segment_data(key_type _beg, key_type _end, data_type* p) :
            begin_key(_beg), end_key(_end), pdata(p) {}
    };
    typedef ::boost::ptr_vector<segment_data> data_array_type;

public:
    struct nonleaf_value_type
    {
        key_type low;   /// low range value (inclusive)
        key_type high;  /// high range value (non-inclusive)
        data_chain_type* data_labels;
    };

    struct leaf_value_type
    {
        key_type key;
        data_chain_type* data_chain;
    };

    struct node : public node_base
    {
        union {
            nonleaf_value_type  value_nonleaf;
            leaf_value_type     value_leaf;
        };

        node(bool _is_leaf) :
            node_base(_is_leaf)
        {
            if (_is_leaf)
                value_leaf.data_chain = NULL;
            else
                value_nonleaf.data_labels = NULL;
        }

        node(const node& r) :
            node_base(r)
        {
        }

        virtual ~node()
        {
            if (is_leaf)
                delete value_leaf.data_chain;
            else
                delete value_nonleaf.data_labels;
        }

        bool equals(const node& r) const
        {
            if (is_leaf != r.is_leaf)
                return false;

            return true;
        }

        virtual void fill_nonleaf_value(const node_base_ptr& left_node, const node_base_ptr& right_node)
        {
            // Parent node should carry the range of all of its child nodes.
            if (left_node)
                value_nonleaf.low  = left_node->is_leaf ? get_node(left_node)->value_leaf.key : get_node(left_node)->value_nonleaf.low;
            else
                // Having a left node is prerequisite.
                return;

            if (right_node)
            {
                if (right_node->is_leaf)
                {
                    // When the child nodes are leaf nodes, the upper bound
                    // must be the value of the node that comes after the
                    // right leaf node (if such node exists).

                    if (right_node->right)
                        value_nonleaf.high = get_node(right_node->right)->value_leaf.key;
                    else
                        value_nonleaf.high = get_node(right_node)->value_leaf.key;
                }
                else
                {
                    value_nonleaf.high = get_node(right_node)->value_nonleaf.high;
                }
            }
            else
                value_nonleaf.high = left_node->is_leaf ? get_node(left_node)->value_leaf.key : get_node(left_node)->value_nonleaf.high;
        }

        virtual node_base* create_new(bool leaf) const
        {
            return new node(leaf);
        }

        virtual node_base* clone() const
        {
            return new node(*this);
        }

#if UNIT_TEST
        virtual void dump_value() const
        {
            ::std::cout << print();
        }

        ::std::string print() const
        {
            ::std::ostringstream os;
            if (is_leaf)
            {
                os << "[" << value_leaf.key << "]";
            }
            else
            {
                os << "[" << value_nonleaf.low << "-" << value_nonleaf.high << ")";
                if (value_nonleaf.data_labels)
                {
                    os << " { ";
                    typename data_chain_type::const_iterator itr, itr_beg = value_nonleaf.data_labels->begin(), itr_end = value_nonleaf.data_labels->end();
                    for (itr = itr_beg; itr != itr_end; ++itr)
                    {
                        if (itr != itr_beg)
                            os << ", ";
                        os << (*itr)->name;
                    }
                    os << " }";
                }
            }
            os << " ";
            return os.str();
        }

        struct printer : public ::std::unary_function<const node*, void>
        {
            void operator() (const node* p) const
            {
                ::std::cout << p->print() << " ";
            }
        };
#endif
    };

    segment_tree();
    ~segment_tree();

    /** 
     * Check whether or not the internal tree is in a valid state.  The tree 
     * must be valid in order to perform searches. 
     * 
     * @return true if the tree is valid, false otherwise.
     */
    bool is_tree_valid() const { return m_valid_tree; }

    /** 
     * Build or re-build tree based on the current set of segments.
     */
    void build_tree();

    /** 
     * Insert a new segment.
     *  
     * @param begin_key begin point of the segment.  The value is inclusive.
     * @param end_key end point of the segment.  The value is non-inclusive. 
     * @param pdata pointer to the data instance associated with this segment. 
     *               Note that <i>the caller must manage the life cycle of the
     *               data instance</i>.
     */
    void insert(key_type begin_key, key_type end_key, data_type* pdata);

    /** 
     * Search the tree and collect all segments that include a specified 
     * point. 
     *  
     * @param point specified point value 
     * @param data_chain doubly-linked list of data instances associated with 
     *                   the segments that include the specified point.
     *  
     * @return true if the search is performed successfully, false if the 
     *         search has ended prematurely due to error conditions.
     */
    bool search(key_type point, data_chain_type& data_chain) const;

    /** 
     * Remove a segment by the data pointer.  This will <i>not</i> invalidate 
     * the tree; however, if you have removed lots of segments, you might want 
     * to re-build the tree to shrink its size. 
     */
    void remove(data_type* pdata);

#if UNIT_TEST
    void dump_tree() const;
    void dump_leaf_nodes() const;
    bool verify_node_lists() const;

    struct leaf_node_check
    {
        key_type key;
        data_chain_type data_chain;
    };

    bool verify_leaf_nodes(const ::std::vector<leaf_node_check>& checks) const;
#endif

private:

    typedef ::std::list<node*> node_list_type;
    typedef ::boost::ptr_map<data_type*, node_list_type> data_node_map_type;

    static node* get_node(const node_base_ptr& base_node)
    { 
        return static_cast<node*>(base_node.get());
    }

    static void create_leaf_node_instances(const ::std::vector<key_type>& keys, node_base_ptr& left, node_base_ptr& right);

    /** 
     * Descend the tree from the root node, and mark appropriate nodes, both 
     * leaf and non-leaf, based on segment's end points.  When marking nodes, 
     * record their positions as a list of node pointers. 
     */
    void descend_tree_and_mark(node* pnode, const segment_data& data, node_list_type* plist);

    void build_leaf_nodes();
    void descend_tree_for_search(key_type point, const node* pnode, data_chain_type& data_chain) const;
    void append_data_chain(data_chain_type& data_chain, const data_chain_type* node_data) const;

    /** 
     * Go through the list of nodes, and remove the specified data pointer 
     * value from the nodes.
     */
    void remove_data_from_nodes(node_list_type* plist, const data_type* pdata);

#if UNIT_TEST
    static bool has_data_pointer(const node_list_type& node_list, const data_type* pdata);
    static void print_leaf_value(const leaf_value_type& v);
#endif

private:
    data_array_type m_segment_data;

    /** 
     * For each data pointer, it keeps track of all nodes, leaf or non-leaf, 
     * that stores the data pointer label.  This data is used when removing 
     * segments by the data pointer value. 
     */
    data_node_map_type m_tagged_node_map;

    node_base_ptr   m_root_node;
    node_base_ptr   m_left_leaf;
    node_base_ptr   m_right_leaf;
    bool m_valid_tree:1;
};

template<typename _Key, typename _Data>
segment_tree<_Key, _Data>::segment_tree() :
    m_valid_tree(false)
{
}

template<typename _Key, typename _Data>
segment_tree<_Key, _Data>::~segment_tree()
{
    disconnect_leaf_nodes(m_left_leaf.get(), m_right_leaf.get());
    clear_tree(m_root_node.get());
    disconnect_node(m_root_node.get());
}

template<typename _Key, typename _Data>
void segment_tree<_Key, _Data>::build_tree()
{
    build_leaf_nodes();
    clear_tree(m_root_node.get());
    m_root_node = ::mdds::build_tree(m_left_leaf);
    
    // Start "inserting" all segments from the root.
    typename data_array_type::const_iterator itr, 
        itr_beg = m_segment_data.begin(), itr_end = m_segment_data.end();

    data_node_map_type tagged_node_map;
    for (itr = itr_beg; itr != itr_end; ++itr)
    {
        data_type* pdata = itr->pdata;
        ::std::pair<typename data_node_map_type::iterator, bool> r = 
            tagged_node_map.insert(pdata, new node_list_type);
        node_list_type* plist = r.first->second;
        descend_tree_and_mark(get_node(m_root_node), *itr, plist);
    }

    m_tagged_node_map.swap(tagged_node_map);
    m_valid_tree = true;
}

template<typename _Key, typename _Data>
void segment_tree<_Key, _Data>::descend_tree_and_mark(
    node* pnode, const segment_data& data, node_list_type* plist)
{
    if (!pnode)
        return;

    if (pnode->is_leaf)
    {
        // This is a leaf node.
        if (pnode->value_leaf.key == data.begin_key)
        {
            // Insertion of begin point.
            leaf_value_type& v = pnode->value_leaf;
            if (!v.data_chain)
                v.data_chain = new data_chain_type;
            v.data_chain->push_back(data.pdata);
            plist->push_back(pnode);
        }
        else if (pnode->value_leaf.key == data.end_key)
        {
            // For insertion of the end point, insert data pointer to the
            // previous node _only when_ the value of the previous node
            // doesn't equal the begin point value.
            node* pprev = get_node(pnode->left);
            if (pprev && pprev->value_leaf.key != data.begin_key)
            {
                leaf_value_type& v = pprev->value_leaf;
                if (!v.data_chain)
                    v.data_chain = new data_chain_type;
                v.data_chain->push_back(data.pdata);
                plist->push_back(pprev);
            }
        }
        return;
    }
    
    if (data.end_key < pnode->value_nonleaf.low || pnode->value_nonleaf.high <= data.begin_key)
        return;

    nonleaf_value_type& v = pnode->value_nonleaf;
    if (data.begin_key <= v.low && v.high < data.end_key)
    {
        // mark this non-leaf node and stop.
        if (!v.data_labels)
            v.data_labels = new data_chain_type;
        v.data_labels->push_back(data.pdata);
        plist->push_back(pnode);
        return;
    }

    descend_tree_and_mark(get_node(pnode->left), data, plist);
    descend_tree_and_mark(get_node(pnode->right), data, plist);
}

template<typename _Key, typename _Data>
void segment_tree<_Key, _Data>::build_leaf_nodes()
{
    using namespace std;

    disconnect_leaf_nodes(m_left_leaf.get(), m_right_leaf.get());

    // In 1st pass, collect unique end-point values and sort them.
    vector<key_type> keys_uniq;
    keys_uniq.reserve(m_segment_data.size()*2);
    typename data_array_type::const_iterator itr, itr_beg = m_segment_data.begin(), itr_end = m_segment_data.end();
    for (itr = itr_beg; itr != itr_end; ++itr)
    {
        keys_uniq.push_back(itr->begin_key);
        keys_uniq.push_back(itr->end_key);
    }

    // sort and remove duplicates.
    sort(keys_uniq.begin(), keys_uniq.end());
    keys_uniq.erase(unique(keys_uniq.begin(), keys_uniq.end()), keys_uniq.end());

    create_leaf_node_instances(keys_uniq, m_left_leaf, m_right_leaf);
}

template<typename _Key, typename _Data>
void segment_tree<_Key, _Data>::create_leaf_node_instances(const ::std::vector<key_type>& keys, node_base_ptr& left, node_base_ptr& right)
{
    if (keys.empty() || keys.size() < 2)
        // We need at least two keys in order to build tree.
        return;

    typename ::std::vector<key_type>::const_iterator itr = keys.begin(), itr_end = keys.end();

    // left-most node
    left.reset(new node(true));
    get_node(left)->value_leaf.key = *itr;

    // move on to next.
    left->right.reset(new node(true));
    node_base_ptr prev_node = left;
    node_base_ptr cur_node = left->right;
    cur_node->left = prev_node;

    for (++itr; itr != itr_end; ++itr)
    {
        get_node(cur_node)->value_leaf.key = *itr;

        // move on to next
        cur_node->right.reset(new node(true));
        prev_node = cur_node;
        cur_node = cur_node->right;
        cur_node->left = prev_node;
    }

    // Remove the excess node.
    prev_node->right.reset();
    right = prev_node;
}

template<typename _Key, typename _Data>
void segment_tree<_Key, _Data>::insert(key_type begin_key, key_type end_key, data_type* pdata)
{
    if (begin_key >= end_key)
        return;

    m_segment_data.push_back(new segment_data(begin_key, end_key, pdata));
    m_valid_tree = false;
}

template<typename _Key, typename _Data>
bool segment_tree<_Key, _Data>::search(key_type point, data_chain_type& data_chain) const
{
    if (!m_valid_tree)
        // Tree is invalidated.
        return false;

    data_chain_type result;
    if (!m_root_node.get())
        // Tree doesn't exist.
        return false;

    descend_tree_for_search(point, get_node(m_root_node), result);
    result.swap(data_chain);
    return true;
}

template<typename _Key, typename _Data>
void segment_tree<_Key, _Data>::remove(data_type* pdata)
{
    typename data_node_map_type::iterator itr = m_tagged_node_map.find(pdata);
    if (itr == m_tagged_node_map.end())
        // the data pointer is not stored in the tree.
        return;

    node_list_type* plist = itr->second;
    if (!plist)
        return;

    remove_data_from_nodes(plist, pdata);
}

template<typename _Key, typename _Data>
void segment_tree<_Key, _Data>::remove_data_from_nodes(node_list_type* plist, const data_type* pdata)
{
    typename node_list_type::iterator itr = plist->begin(), itr_end = plist->end();
    for (; itr != itr_end; ++itr)
    {
        data_chain_type* chain = NULL;
        node* p = *itr;
        if (p->is_leaf)
            chain = p->value_leaf.data_chain;
        else
            chain = p->value_nonleaf.data_labels;

        if (!chain)
            continue;

        chain->remove(pdata);
    }
}

template<typename _Key, typename _Data>
void segment_tree<_Key, _Data>::descend_tree_for_search(key_type point, const node* pnode, data_chain_type& data_chain) const
{
    if (!pnode)
        // This should never happen, but just in case.
        return;

    if (pnode->is_leaf)
    {
        append_data_chain(data_chain, pnode->value_leaf.data_chain);
        return;
    }

    const nonleaf_value_type& v = pnode->value_nonleaf;
    if (point < v.low || v.high <= point)
        // Query point is out-of-range.
        return;

    append_data_chain(data_chain, pnode->value_nonleaf.data_labels);

    // Check the left child node first, then the right one.
    node* pchild = get_node(pnode->left);
    assert(pchild->is_leaf == pnode->right->is_leaf);
    if (pchild->is_leaf)
    {
        // The child node are leaf nodes.
        const leaf_value_type& vleft = pchild->value_leaf;
        const leaf_value_type& vright = get_node(pnode->right)->value_leaf;
        if (point < vleft.key)
        {
            // Out-of-range.  Nothing more to do.
            return;
        }

        if (vright.key <= point)
            // Follow the right node.
            pchild = get_node(pnode->right);
    }
    else
    {
        const nonleaf_value_type& vleft = pchild->value_nonleaf;
        if (point < vleft.low)
        {
            // Out-of-range.  Nothing more to do.
            return;
        }
        if (vleft.high <= point)
            // Follow the right child.
            pchild = get_node(pnode->right);

        assert(pchild->value_nonleaf.low <= point && point < pchild->value_nonleaf.high);
    }
    descend_tree_for_search(point, pchild, data_chain);
}

template<typename _Key, typename _Data>
void segment_tree<_Key, _Data>::append_data_chain(data_chain_type& data_chain, const data_chain_type* node_data) const
{
    if (!node_data)
        return;

    copy(node_data->begin(), node_data->end(), back_inserter(data_chain));
}

#if UNIT_TEST
template<typename _Key, typename _Data>
void segment_tree<_Key, _Data>::dump_tree() const
{
    using ::std::cout;
    using ::std::endl;

    if (!m_valid_tree)
        assert(!"attempted to dump an invalid tree!");

    size_t node_count = ::mdds::dump_tree(m_root_node);
    size_t node_instance_count = node_base::get_instance_count();

    cout << "tree node count = " << node_count << "    node instance count = " << node_instance_count << endl;
    assert(node_count == node_instance_count);
}

template<typename _Key, typename _Data>
void segment_tree<_Key, _Data>::dump_leaf_nodes() const
{
    using ::std::cout;
    using ::std::endl;

    cout << "------------------------------------------" << endl;

    node* p = get_node(m_left_leaf);
    while (p)
    {
        print_leaf_value(p->value_leaf);
        p = get_node(p->right);
    }
    cout << endl << "  node instance count = " << node_base::get_instance_count() << endl;
}

template<typename _Key, typename _Data>
bool segment_tree<_Key, _Data>::verify_node_lists() const
{
    using namespace std;

    typename data_node_map_type::const_iterator 
        itr = m_tagged_node_map.begin(), itr_end = m_tagged_node_map.end();
    for (; itr != itr_end; ++itr)
    {
        // Print stored nodes.
        cout << "node list " << itr->first->name << ": ";
        const node_list_type* plist = itr->second;
        assert(plist);
        typename node::printer func;
        for_each(plist->begin(), plist->end(), func);
        cout << endl;

        // Verify that all of these nodes have the data pointer.
        if (!has_data_pointer(*plist, itr->first))
            return false;
    }
    return true;
}

template<typename _Key, typename _Data>
bool segment_tree<_Key, _Data>::verify_leaf_nodes(const ::std::vector<leaf_node_check>& checks) const
{
    node* cur_node = get_node(m_left_leaf);
    typename ::std::vector<leaf_node_check>::const_iterator itr = checks.begin(), itr_end = checks.end();
    for (; itr != itr_end; ++itr)
    {
        if (!cur_node)
            // Position past the right-mode node.  Invalid.
            return false;

        if (cur_node->value_leaf.key != itr->key)
            // Key values differ.
            return false;

        if (itr->data_chain.empty())
        {
            if (cur_node->value_leaf.data_chain)
                // The data chain should be empty (i.e. the pointer should be NULL).
                return false;
        }
        else
        {
            if (!cur_node->value_leaf.data_chain)
                // This node should have data pointers!
                return false;

            typename data_chain_type::const_iterator itr1 = itr->data_chain.begin();
            typename data_chain_type::const_iterator itr1_end = itr->data_chain.end();
            typename data_chain_type::const_iterator itr2 = cur_node->value_leaf.data_chain->begin();
            typename data_chain_type::const_iterator itr2_end = cur_node->value_leaf.data_chain->end();
            for (; itr1 != itr1_end; ++itr1, ++itr2)
            {
                if (itr2 == itr2_end)
                    // Data chain in the node finished early.
                    return false;

                if (*itr1 != *itr2)
                    // Data pointers differ.
                    return false;
            }
            if (itr2 != itr2_end)
                // There are more data pointers in the node.
                return false;
        }

        cur_node = get_node(cur_node->right);
    }

    if (cur_node)
        // At this point, we expect the current node to be at the position
        // past the right-most node, which is NULL.
        return false;

    return true;
}

template<typename _Key, typename _Data>
bool segment_tree<_Key, _Data>::has_data_pointer(const node_list_type& node_list, const data_type* pdata)
{
    using namespace std;

    typename node_list_type::const_iterator
        itr = node_list.begin(), itr_end = node_list.end();

    for (; itr != itr_end; ++itr)
    {
        // Check each node, and make sure each node has the pdata pointer
        // listed.
        const node* pnode = *itr;
        const data_chain_type* chain = NULL;
        if (pnode->is_leaf)
            chain = pnode->value_leaf.data_chain;
        else
            chain = pnode->value_nonleaf.data_labels;

        if (!chain)
            return false;
        
        if (find(chain->begin(), chain->end(), pdata) == chain->end())
            return false;
    }
    return true;
}

template<typename _Key, typename _Data>
void segment_tree<_Key, _Data>::print_leaf_value(const leaf_value_type& v)
{
    using namespace std;
    cout << v.key << ": { ";
    if (v.data_chain)
    {
        const data_chain_type* pchain = v.data_chain;
        typename data_chain_type::const_iterator itr, itr_beg = pchain->begin(), itr_end = pchain->end();
        for (itr = itr_beg; itr != itr_end; ++itr)
        {
            if (itr != itr_beg)
                cout << ", ";
            cout << (*itr)->name;
        }
    }
    cout << " }" << endl;
}
#endif

}

#endif
