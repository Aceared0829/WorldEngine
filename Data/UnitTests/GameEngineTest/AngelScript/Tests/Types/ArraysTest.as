#include "../TestFramework.as"

void ExecuteTests()
{
    // typedef to basic type
    {
        WUInt32 test = 5;
        array<WUInt32> elements;

        W_TEST_BOOL(elements.IsEmpty());
        W_TEST_BOOL(elements.GetCount() == 0);
        W_TEST_BOOL(!elements.Contains(test));
        W_TEST_INT(elements.IndexOf(test), -1);
        elements.PushBack(test);
        W_TEST_BOOL(!elements.IsEmpty());
        W_TEST_BOOL(elements.GetCount() == 1);
        W_TEST_BOOL(elements.Contains(test));
        W_TEST_INT(elements.IndexOf(test), 0);
        W_TEST_BOOL(elements[0] == test);
    }

    // pod value type
    {
        WGameObjectHandle test;
        array<WGameObjectHandle> elements;

        W_TEST_BOOL(elements.IsEmpty());
        W_TEST_BOOL(elements.GetCount() == 0);
        W_TEST_BOOL(!elements.Contains(test));
        W_TEST_INT(elements.IndexOf(test), -1);
        elements.PushBack(test);
        W_TEST_BOOL(!elements.IsEmpty());
        W_TEST_BOOL(elements.GetCount() == 1);
        W_TEST_BOOL(elements.Contains(test));
        W_TEST_INT(elements.IndexOf(test), 0);
        W_TEST_BOOL(elements[0] == test);
    }

    // Bigger pod value type
    {
        WVec4 test(1.0f, 2.0f, 3.0f, 4.0f);
        WVec4 test2(5.0f, 6.0f, 7.0f, 8.0f);
        array<WVec4> elements;

        W_TEST_BOOL(elements.IsEmpty());
        W_TEST_BOOL(elements.GetCount() == 0);
        W_TEST_BOOL(!elements.Contains(test));
        W_TEST_BOOL(!elements.Contains(test2));
        W_TEST_INT(elements.IndexOf(test), -1);
        W_TEST_INT(elements.IndexOf(test2), -1);

        elements.PushBack(test);
        W_TEST_BOOL(!elements.IsEmpty());
        W_TEST_BOOL(elements.GetCount() == 1);
        W_TEST_BOOL(elements.Contains(test));
        W_TEST_INT(elements.IndexOf(test), 0);
        W_TEST_BOOL(elements[0] == test);
        W_TEST_BOOL(elements[0] != test2);

        elements.PushBack(test2);
        W_TEST_BOOL(!elements.IsEmpty());
        W_TEST_BOOL(elements.GetCount() == 2);
        W_TEST_BOOL(elements.Contains(test));
        W_TEST_BOOL(elements.Contains(test2));
        W_TEST_INT(elements.IndexOf(test), 0);
        W_TEST_INT(elements.IndexOf(test2), 1);
        W_TEST_BOOL(elements[0] == test);
        W_TEST_BOOL(elements[1] == test2);
        W_TEST_BOOL(elements[0] != test2);
        W_TEST_BOOL(elements[1] != test);

        elements.PushBack(WVec4(9.0f, 10.0f, 11.0f, 12.0f));
        W_TEST_BOOL(elements.Contains(WVec4(9.0f, 10.0f, 11.0f, 12.0f)));
        W_TEST_BOOL(elements[2] == WVec4(9.0f, 10.0f, 11.0f, 12.0f));
    }

    // non-pod value type
    {
        WString test = "Test";
        array<WString> elements;

        W_TEST_BOOL(elements.IsEmpty());
        W_TEST_BOOL(elements.GetCount() == 0);
        W_TEST_BOOL(!elements.Contains(test));
        W_TEST_INT(elements.IndexOf(test), -1);
        elements.PushBack(test);
        W_TEST_BOOL(!elements.IsEmpty());
        W_TEST_BOOL(elements.GetCount() == 1);
        W_TEST_BOOL(elements.Contains(test));
        W_TEST_INT(elements.IndexOf(test), 0);
        W_TEST_BOOL(elements[0] == test);
    }

    // WStringView
    {
        WStringView test = "Test";
        array<WStringView> elements;

        W_TEST_BOOL(elements.IsEmpty());
        W_TEST_BOOL(elements.GetCount() == 0);
        W_TEST_BOOL(!elements.Contains(test));
        W_TEST_INT(elements.IndexOf(test), -1);
        elements.PushBack(test);
        W_TEST_BOOL(!elements.IsEmpty());
        W_TEST_BOOL(elements.GetCount() == 1);
        W_TEST_BOOL(elements.Contains(test));
        W_TEST_INT(elements.IndexOf(test), 0);
        W_TEST_BOOL(elements[0] == test);
    }
}