#include <radix_tree/radix_tree.hpp>
#include <radix_tree/statistics.hpp>

#include <string>

#include <boost/property_tree/json_parser.hpp>

#include <gtest/gtest.h>

using RadixTree = radix_tree::tree<std::string, size_t>;

TEST(RadixTree, Elementary)
{
    RadixTree t;

    // root
    EXPECT_EQ(1, t.node_count());
    // root-(foo)
    t.insert("foo", 0);
    EXPECT_EQ(2, t.node_count());
    EXPECT_EQ(1, t.value_count());

    auto cnt = 0u;
    std::string child_key;
    t.traverse("foo",
        [&](auto&, auto const& key) {
            ++cnt;
            child_key = key;
            return false;
        }
    );

    EXPECT_EQ(1, cnt);
    EXPECT_EQ("foo", child_key);
}

TEST(RadixTree, AppendSuffixNode)
{
    RadixTree t;

    // root-(abc)
    t.insert("abc", 0);
    // root-(abc)-(d)
    t.insert("abcd", 1);
    EXPECT_EQ(3, t.node_count());
    EXPECT_EQ(2, t.value_count());

    auto cnt = 0u;
    std::vector<std::string> child_keys;
    t.traverse("abcd",
        [&](auto&, auto const& key) {
            ++cnt;
            child_keys.push_back(key);
            return false;
        }
    );

    EXPECT_EQ(2, cnt);
    ASSERT_EQ(2, child_keys.size());
    EXPECT_EQ("abc", child_keys[0]);
    EXPECT_EQ("d", child_keys[1]);
}

TEST(RadixTree, SplitNode)
{
    RadixTree t;

    // root-(abcd)
    t.insert("abcd", 1);
    EXPECT_EQ(2, t.node_count());
    EXPECT_EQ(1, t.value_count());

    // root-(abc)-(d)
    t.insert("abc", 0);
    EXPECT_EQ(3, t.node_count());
    EXPECT_EQ(2, t.value_count());

    auto cnt = 0u;
    std::vector<std::string> child_keys;
    t.traverse("abcd",
        [&](auto&, auto const& key) {
            ++cnt;
            child_keys.push_back(key);
            return false;
        }
    );

    EXPECT_EQ(2, cnt);
    EXPECT_EQ(2, child_keys.size());
    EXPECT_EQ("abc", child_keys[0]);
    EXPECT_EQ("d", child_keys[1]);
}

TEST(RadixTree, SplitPrefixNode)
{
    // root
    RadixTree t;

    // root-(abcd)
    t.insert("abcd", 0);
    EXPECT_EQ(2, t.node_count());
    EXPECT_EQ(1, t.value_count());

    // root-(abc)-(d)
    t.insert("abc", 1);
    EXPECT_EQ(3, t.node_count());
    EXPECT_EQ(2, t.value_count());

    // root-a-+-(bc)-(d)
    //        +-(de)
    t.insert("ade", 2);
    EXPECT_EQ(5, t.node_count());
    EXPECT_EQ(3, t.value_count());

    {
        auto cnt = 0u;
        std::vector<std::string> child_keys;
        t.traverse("abcd",
            [&](auto&, auto const& key) {
                ++cnt;
                child_keys.push_back(key);
                return false;
            }
        );

        EXPECT_EQ(3, cnt);
        EXPECT_EQ(3, child_keys.size());
        EXPECT_EQ("a", child_keys[0]);
        EXPECT_EQ("bc", child_keys[1]);
        EXPECT_EQ("d", child_keys[2]);
    }
    {
        auto cnt = 0u;
        std::vector<std::string> child_keys;
        t.traverse("ade",
            [&](auto&, auto const& key) {
                ++cnt;
                child_keys.push_back(key);
                return false;
            }
        );

        EXPECT_EQ(2, cnt);
        EXPECT_EQ(2, child_keys.size());
        EXPECT_EQ("a", child_keys[0]);
        EXPECT_EQ("de", child_keys[1]);
    }
}

TEST(RadixTree, CaseInsensitive)
{
    {
        RadixTree t;

        t.insert("FOO", 0);

        auto cnt = 0u;
        std::string child_key;
        t.traverse("foo",
            [&](auto&, auto const& key) {
                ++cnt;
                child_key = key;
                return false;
            }
        );

        ASSERT_EQ(1, cnt);
        EXPECT_EQ("FOO", child_key);
    }
    {
        RadixTree t;

        t.insert("foo", 0);

        auto cnt = 0u;
        std::string child_key;
        t.traverse("FOO",
            [&](auto&, auto const& key) {
                ++cnt;
                child_key = key;
                return false;
            }
        );

        ASSERT_EQ(1, cnt);
        EXPECT_EQ("foo", child_key);
    }
    {
        RadixTree t;

        t.insert("foo", 0);
        t.insert("FOO", 1);

        auto cnt = 0u;
        std::string child_key;
        t.traverse("foo",
            [&](auto&, auto const& key) {
                ++cnt;
                child_key = key;
                return false;
            }
        );

        ASSERT_EQ(1, cnt);
        EXPECT_EQ("foo", child_key);
    }
}

TEST(RadixTree, DuplicatedKey)
{
    RadixTree t;

    // root---(a)---b-+-(c)
    //                +-(d)
    t.insert("a", 0);
    t.insert("abc", 1);
    t.insert("abd", 2);
    EXPECT_EQ(5, t.node_count());
    EXPECT_EQ(3, t.value_count());

    // root--(a)--(b)-+-(c)
    //                +-(d)
    t.insert("ab", 3);
    EXPECT_EQ(5, t.node_count());
    EXPECT_EQ(4, t.value_count());
}

#include <boost/range/iterator_range.hpp>
using StringRange = boost::iterator_range<const char*>;

inline StringRange
operator"" _r(const char *str, const size_t len)
{
    return { str, str + len };
}

TEST(RadixTree, UseRangeAsKey)
{
    using RadixTreeR = radix_tree::tree<StringRange, size_t>;

    // root
    RadixTreeR t;

    // root-(abcd)
    t.insert("abcd"_r, 0);
    EXPECT_EQ(2, t.node_count());
    EXPECT_EQ(1, t.value_count());

    // root-(abc)-(d)
    t.insert("abc"_r, 1);
    EXPECT_EQ(3, t.node_count());
    EXPECT_EQ(2, t.value_count());

    // root-a-+-(bc)-(d)
    //        +-(de)
    t.insert("ade"_r, 2);
    EXPECT_EQ(5, t.node_count());
    EXPECT_EQ(3, t.value_count());

    {
        auto cnt = 0u;
        std::vector<RadixTreeR::key_t> child_keys;
        t.traverse("abcd"_r,
            [&](auto&, auto const& key) {
                ++cnt;
                child_keys.push_back(key);
                return false;
            }
        );

        EXPECT_EQ(3, cnt);
        EXPECT_EQ(3, child_keys.size());
        EXPECT_EQ("a"_r, child_keys[0]);
        EXPECT_EQ("bc"_r, child_keys[1]);
        EXPECT_EQ("d"_r, child_keys[2]);
    }
    {
        auto cnt = 0u;
        std::vector<RadixTreeR::key_t> child_keys;
        t.traverse("ade"_r,
            [&](auto&, auto const& key) {
                ++cnt;
                child_keys.push_back(key);
                return false;
            }
        );

        EXPECT_EQ(2, cnt);
        EXPECT_EQ(2, child_keys.size());
        EXPECT_EQ("a"_r, child_keys[0]);
        EXPECT_EQ("de"_r, child_keys[1]);
    }
}

TEST(RadixTree, Foo)
{
    RadixTree t;

    t.insert("adblocks", 0);
    EXPECT_EQ(2, t.node_count());
    EXPECT_EQ(1, t.value_count());

    auto cnt = 0u;
    t.traverse("adblock.org",
        [&](auto&, auto const& key) {
            std::cout << key << "\n";
            ++cnt;
            return false;
        }
    );

    EXPECT_EQ(0, cnt);
}

TEST(RadixTree, Clear)
{
    RadixTree t;

    t.insert("adblocks", 0);
    t.insert("foo", 0);
    EXPECT_EQ(3, t.node_count());
    EXPECT_EQ(2, t.value_count());

    t.clear();
    EXPECT_EQ(1, t.node_count());
    EXPECT_EQ(0, t.value_count());

    t.insert("abc", 0);
    t.insert("abcd", 0);
    EXPECT_EQ(3, t.node_count());
    EXPECT_EQ(2, t.value_count());

    t.clear();
    EXPECT_EQ(1, t.node_count());
    EXPECT_EQ(0, t.value_count());
}

TEST(RadixTree, Statistics)
{
    RadixTree t;

    t.insert("adblocks", 0);
    t.insert("foo", 0);
    t.insert("abc", 0);
    t.insert("abcd", 0);

    auto const &stats = statistics(t);

    namespace bpt = boost::property_tree;

    EXPECT_NO_THROW(EXPECT_EQ(3, stats.get<size_t>("Number of leaf")));
    EXPECT_NO_THROW(EXPECT_EQ(3, stats.get<size_t>("Number of branch")));
    EXPECT_NO_THROW(EXPECT_EQ(6, stats.get<size_t>("Number of nodes")));
    EXPECT_NO_THROW(EXPECT_EQ(4, stats.get<size_t>("Number of values")));

    bpt::ptree child;
    ASSERT_NO_THROW(child = stats.get_child("Branches by children"));
    EXPECT_EQ(2, child.size());
    EXPECT_NO_THROW(EXPECT_EQ(1, child.get<size_t>("1")));
    EXPECT_NO_THROW(EXPECT_EQ(2, child.get<size_t>("2")));

    ASSERT_NO_THROW(child = stats.get_child("Branches by level"));
    EXPECT_EQ(3, child.size());
    EXPECT_NO_THROW(EXPECT_EQ(1, child.get<size_t>("0")));
    EXPECT_NO_THROW(EXPECT_EQ(1, child.get<size_t>("1")));
    EXPECT_NO_THROW(EXPECT_EQ(1, child.get<size_t>("2")));

    ASSERT_NO_THROW(child = stats.get_child("Leaves by level"));
    EXPECT_EQ(3, child.size());
    EXPECT_NO_THROW(EXPECT_EQ(1, child.get<size_t>("1")));
    EXPECT_NO_THROW(EXPECT_EQ(1, child.get<size_t>("2")));
    EXPECT_NO_THROW(EXPECT_EQ(1, child.get<size_t>("3")));

    ASSERT_NO_THROW(child = stats.get_child("Nodes by values"));
    EXPECT_EQ(2, child.size());
    EXPECT_NO_THROW(EXPECT_EQ(2, child.get<size_t>("0")));
    EXPECT_NO_THROW(EXPECT_EQ(4, child.get<size_t>("1")));
}

TEST(RadixTree, EmptyKey)
{
    RadixTree t;

    // root
    EXPECT_EQ(1, t.node_count());
    EXPECT_EQ(0, t.value_count());

    // (root)
    t.insert("", 0);
    EXPECT_EQ(1, t.node_count());
    EXPECT_EQ(1, t.value_count());

    // empty key will match all keys.
    auto cnt = 0u;
    std::string child_key;
    t.traverse("foo",
        [&](auto&, auto const& key) {
            ++cnt;
            child_key = key;
            return false;
        }
    );

    ASSERT_EQ(1, cnt);
    EXPECT_EQ("", child_key);
}

TEST(RadixTree, BugFix_CantClearValueOfEmptyRoot)
{
    RadixTree t;

    // root
    EXPECT_EQ(1, t.node_count());
    EXPECT_EQ(0, t.value_count());

    // (root)
    t.insert("", 0);
    t.insert("", 1);
    EXPECT_EQ(1, t.node_count());
    EXPECT_EQ(2, t.value_count());

    t.clear();
    EXPECT_EQ(1, t.node_count());
    EXPECT_EQ(0, t.value_count());
}
