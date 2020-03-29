#include "CppUnitTest.h"

#include "ui\viewmodels\MemorySearchViewModel.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockConsoleContext.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockEmulatorContext.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockWindowManager.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Microsoft {
namespace VisualStudio {
namespace CppUnitTestFramework {

template<>
std::wstring ToString<ra::ui::viewmodels::MemorySearchViewModel::SearchType>(
    const ra::ui::viewmodels::MemorySearchViewModel::SearchType& nSearchType)
{
    switch (nSearchType)
    {
        case ra::ui::viewmodels::MemorySearchViewModel::SearchType::FourBit:
            return L"FourBit";
        case ra::ui::viewmodels::MemorySearchViewModel::SearchType::EightBit:
            return L"EightBit";
        case ra::ui::viewmodels::MemorySearchViewModel::SearchType::SixteenBit:
            return L"SixteenBit";
        default:
            return std::to_wstring(static_cast<int>(nSearchType));
    }
}

template<>
std::wstring ToString<ra::ui::viewmodels::MemorySearchViewModel::ValueType>(
    const ra::ui::viewmodels::MemorySearchViewModel::ValueType& nValueType)
{
    switch (nValueType)
    {
        case ra::ui::viewmodels::MemorySearchViewModel::ValueType::Constant:
            return L"Constant";
        case ra::ui::viewmodels::MemorySearchViewModel::ValueType::LastKnownValue:
            return L"LastKnownValue";
        default:
            return std::to_wstring(static_cast<int>(nValueType));
    }
}

} // namespace CppUnitTestFramework
} // namespace VisualStudio
} // namespace Microsoft

namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

TEST_CLASS(MemorySearchViewModel_Tests)
{
private:
    class MemorySearchViewModelHarness : public MemorySearchViewModel
    {
    public:
        ra::data::mocks::MockConsoleContext mockConsoleContext;
        ra::data::mocks::MockEmulatorContext mockEmulatorContext;
        ra::data::mocks::MockGameContext mockGameContext;
        ra::services::mocks::MockConfiguration mockConfiguration;
        ra::ui::mocks::MockDesktop mockDesktop;
        ra::ui::viewmodels::mocks::MockWindowManager mockWindowManager;

        std::array<unsigned char, 32> memory{};

        void InitializeMemory()
        {
            for (size_t i = 0; i < memory.size(); ++i)
                memory.at(i) = gsl::narrow_cast<unsigned char>(i);

            mockEmulatorContext.MockMemory(memory);
        }
    };

    void AssertRow(MemorySearchViewModelHarness& search, gsl::index nRow, ra::ByteAddress nAddress,
        const wchar_t* sAddress, const wchar_t* sCurrentValue, const wchar_t* sPreviousValue)
    {
        auto* pRow = search.Results().GetItemAt(nRow);
        Assert::IsNotNull(pRow);
        Ensures(pRow != nullptr);

        Assert::AreEqual(std::wstring(sAddress), pRow->GetAddress());
        Assert::AreEqual(std::wstring(sCurrentValue), pRow->GetCurrentValue());
        Assert::AreEqual(std::wstring(sPreviousValue), pRow->GetPreviousValue());
        Assert::AreEqual(nAddress, pRow->nAddress);
    }

public:
    TEST_METHOD(TestInitialValues)
    {
        MemorySearchViewModelHarness search;

        Assert::AreEqual({ 0U }, search.Results().Count());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual({ 0U }, search.GetResultCount());

        Assert::AreEqual(std::wstring(L""), search.GetFilterRange());
        Assert::AreEqual(std::wstring(L""), search.GetFilterValue());

        Assert::AreEqual({ 3U }, search.SearchTypes().Count());
        Assert::AreEqual((int)MemorySearchViewModel::SearchType::FourBit, search.SearchTypes().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L"4-bit"), search.SearchTypes().GetItemAt(0)->GetLabel());
        Assert::AreEqual((int)MemorySearchViewModel::SearchType::EightBit, search.SearchTypes().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"8-bit"), search.SearchTypes().GetItemAt(1)->GetLabel());
        Assert::AreEqual((int)MemorySearchViewModel::SearchType::SixteenBit, search.SearchTypes().GetItemAt(2)->GetId());
        Assert::AreEqual(std::wstring(L"16-bit"), search.SearchTypes().GetItemAt(2)->GetLabel());
        Assert::AreEqual(MemorySearchViewModel::SearchType::EightBit, search.GetSearchType());

        Assert::AreEqual({ 6U }, search.ComparisonTypes().Count());
        Assert::AreEqual((int)ComparisonType::Equals, search.ComparisonTypes().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L"="), search.ComparisonTypes().GetItemAt(0)->GetLabel());
        Assert::AreEqual((int)ComparisonType::LessThan, search.ComparisonTypes().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"<"), search.ComparisonTypes().GetItemAt(1)->GetLabel());
        Assert::AreEqual((int)ComparisonType::LessThanOrEqual, search.ComparisonTypes().GetItemAt(2)->GetId());
        Assert::AreEqual(std::wstring(L"<="), search.ComparisonTypes().GetItemAt(2)->GetLabel());
        Assert::AreEqual((int)ComparisonType::GreaterThan, search.ComparisonTypes().GetItemAt(3)->GetId());
        Assert::AreEqual(std::wstring(L">"), search.ComparisonTypes().GetItemAt(3)->GetLabel());
        Assert::AreEqual((int)ComparisonType::GreaterThanOrEqual, search.ComparisonTypes().GetItemAt(4)->GetId());
        Assert::AreEqual(std::wstring(L">="), search.ComparisonTypes().GetItemAt(4)->GetLabel());
        Assert::AreEqual((int)ComparisonType::NotEqualTo, search.ComparisonTypes().GetItemAt(5)->GetId());
        Assert::AreEqual(std::wstring(L"!="), search.ComparisonTypes().GetItemAt(5)->GetLabel());
        Assert::AreEqual(ComparisonType::Equals, search.GetComparisonType());

        Assert::AreEqual({ 2U }, search.ValueTypes().Count());
        Assert::AreEqual((int)MemorySearchViewModel::ValueType::Constant, search.ValueTypes().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L"Constant"), search.ValueTypes().GetItemAt(0)->GetLabel());
        Assert::AreEqual((int)MemorySearchViewModel::ValueType::LastKnownValue, search.ValueTypes().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"Last Known Value"), search.ValueTypes().GetItemAt(1)->GetLabel());
        Assert::AreEqual(MemorySearchViewModel::ValueType::LastKnownValue, search.GetValueType());

        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"0/0"), search.GetSelectedPage());

        Assert::AreEqual(std::wstring(L""), search.GetFilterSummary());
    }

    TEST_METHOD(TestBeginNewSearchEightBit)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();

        search.BeginNewSearch();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 32U }, search.GetResultCount());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual(std::wstring(L"New 8-bit Search"), search.GetFilterSummary());
    }

    TEST_METHOD(TestBeginNewSearchSixteenBit)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.SetSearchType(MemorySearchViewModel::SearchType::SixteenBit);

        search.BeginNewSearch();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 31U }, search.GetResultCount());
        Assert::AreEqual(MemSize::SixteenBit, search.ResultMemSize());
        Assert::AreEqual(std::wstring(L"New 16-bit Search"), search.GetFilterSummary());
    }

    TEST_METHOD(TestBeginNewSearchFourBit)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.SetSearchType(MemorySearchViewModel::SearchType::FourBit);

        search.BeginNewSearch();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 64U }, search.GetResultCount());
        Assert::AreEqual(MemSize::Nibble_Lower, search.ResultMemSize());
        Assert::AreEqual(std::wstring(L"New 4-bit Search"), search.GetFilterSummary());
    }

    TEST_METHOD(TestApplyFilterEightBitConstantDecimal)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.BeginNewSearch();

        search.SetComparisonType(ComparisonType::Equals);
        search.SetValueType(MemorySearchViewModel::ValueType::Constant);
        search.SetFilterValue(L"12");

        search.ApplyFilter();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 1U }, search.GetResultCount());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual(std::wstring(L"Filter: = 12"), search.GetFilterSummary());

        Assert::AreEqual({ 1U }, search.Results().Count());
        AssertRow(search, 0, 12U, L"0x000c", L"0x0c", L"0x0c");
    }

    TEST_METHOD(TestApplyFilterEightBitConstantHex)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.BeginNewSearch();

        search.SetComparisonType(ComparisonType::Equals);
        search.SetValueType(MemorySearchViewModel::ValueType::Constant);
        search.SetFilterValue(L"0x0C");

        search.ApplyFilter();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 1U }, search.GetResultCount());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual(std::wstring(L"Filter: = 0x0C"), search.GetFilterSummary());

        Assert::AreEqual({ 1U }, search.Results().Count());
        AssertRow(search, 0, 12U, L"0x000c", L"0x0c", L"0x0c");
    }

    TEST_METHOD(TestApplyFilterEightBitConstantInvalid)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.BeginNewSearch();

        search.SetComparisonType(ComparisonType::Equals);
        search.SetValueType(MemorySearchViewModel::ValueType::Constant);
        search.SetFilterValue(L"banana");

        bool bSawDialog = false;
        search.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bSawDialog](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bSawDialog = true;
            Assert::AreEqual(std::wstring(L"Invalid filter value"), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::OK;
        });

        search.ApplyFilter();
        Assert::IsTrue(bSawDialog);

        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 32U }, search.GetResultCount());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual(std::wstring(L"New 8-bit Search"), search.GetFilterSummary());
        Assert::AreEqual({ 0U }, search.Results().Count());
    }

    TEST_METHOD(TestApplyFilterEightBitLastKnown)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.BeginNewSearch();

        search.SetComparisonType(ComparisonType::NotEqualTo);
        search.SetValueType(MemorySearchViewModel::ValueType::LastKnownValue);
        search.memory.at(12) = 9;

        search.ApplyFilter();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 1U }, search.GetResultCount());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual(std::wstring(L"Filter: != Last Known Value"), search.GetFilterSummary());

        Assert::AreEqual({ 1U }, search.Results().Count());
        AssertRow(search, 0, 12U, L"0x000c", L"0x09", L"0x0c");

        // if results don't change and filter doesn't change, don't generate a new page
        search.memory.at(12) = 7;
        search.ApplyFilter();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 1U }, search.GetResultCount());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual(std::wstring(L"Filter: != Last Known Value"), search.GetFilterSummary());

        Assert::AreEqual({ 1U }, search.Results().Count());

        // NOTE: current value is still old value. it will be updated by DoFrame, and normally memory would only be updated by DoFrame as well
        AssertRow(search, 0, 12U, L"0x000c", L"0x09", L"0x0c");
    }

    TEST_METHOD(TestApplyFilterEightBitLastKnownNoChange)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.BeginNewSearch();

        search.SetComparisonType(ComparisonType::NotEqualTo);
        search.SetValueType(MemorySearchViewModel::ValueType::LastKnownValue);
        search.memory.at(12) = 9;

        search.ApplyFilter();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 1U }, search.GetResultCount());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual(std::wstring(L"Filter: != Last Known Value"), search.GetFilterSummary());

        Assert::AreEqual({ 1U }, search.Results().Count());
        AssertRow(search, 0, 12U, L"0x000c", L"0x09", L"0x0c");

        // if results don't change and filter does change, generate a new page
        search.SetComparisonType(ComparisonType::Equals);
        search.ApplyFilter();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"2/2"), search.GetSelectedPage());
        Assert::AreEqual({ 1U }, search.GetResultCount());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual(std::wstring(L"Filter: = Last Known Value"), search.GetFilterSummary());

        Assert::AreEqual({ 1U }, search.Results().Count());
        AssertRow(search, 0, 12U, L"0x000c", L"0x09", L"0x09");
    }

    TEST_METHOD(TestDoFrameEightBit)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.BeginNewSearch();
        search.SetComparisonType(ComparisonType::Equals);
        search.SetValueType(MemorySearchViewModel::ValueType::LastKnownValue);
        search.ApplyFilter();

        search.memory.at(2) = 9;
        search.DoFrame();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 32U }, search.GetResultCount());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());

        Assert::IsTrue(search.Results().Count() > 3);
        AssertRow(search, 0, 0U, L"0x0000", L"0x00", L"0x00");
        AssertRow(search, 1, 1U, L"0x0001", L"0x01", L"0x01");
        AssertRow(search, 2, 2U, L"0x0002", L"0x09", L"0x02");
    }

    TEST_METHOD(TestDoFrameSixteenBit)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.SetSearchType(MemorySearchViewModel::SearchType::SixteenBit);
        search.BeginNewSearch();
        search.SetComparisonType(ComparisonType::Equals);
        search.SetValueType(MemorySearchViewModel::ValueType::LastKnownValue);
        search.ApplyFilter();

        search.memory.at(2) = 9;
        search.DoFrame();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 31U }, search.GetResultCount());
        Assert::AreEqual(MemSize::SixteenBit, search.ResultMemSize());

        Assert::IsTrue(search.Results().Count() > 3);
        AssertRow(search, 0, 0U, L"0x0000", L"0x0100", L"0x0100");
        AssertRow(search, 1, 1U, L"0x0001", L"0x0901", L"0x0201");
        AssertRow(search, 2, 2U, L"0x0002", L"0x0309", L"0x0302");
    }

    TEST_METHOD(TestDoFrameFourBit)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.SetSearchType(MemorySearchViewModel::SearchType::FourBit);
        search.BeginNewSearch();
        search.SetComparisonType(ComparisonType::Equals);
        search.SetValueType(MemorySearchViewModel::ValueType::LastKnownValue);
        search.ApplyFilter();

        search.memory.at(1) = 9;
        search.DoFrame();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 64U }, search.GetResultCount());
        Assert::AreEqual(MemSize::Nibble_Lower, search.ResultMemSize());

        Assert::IsTrue(search.Results().Count() > 3);
        AssertRow(search, 0, 0U, L"0x0000L", L"0x0", L"0x0");
        AssertRow(search, 1, 1U, L"0x0000U", L"0x0", L"0x0");
        AssertRow(search, 2, 2U, L"0x0001L", L"0x9", L"0x1");
    }

    TEST_METHOD(TestNextPagePreviousPage)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.BeginNewSearch();

        search.SetComparisonType(ComparisonType::LessThan);
        search.SetValueType(MemorySearchViewModel::ValueType::Constant);
        search.SetFilterValue(L"8");

        search.ApplyFilter();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual(std::wstring(L"Filter: < 8"), search.GetFilterSummary());
        Assert::AreEqual({ 8U }, search.GetResultCount());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual({ 8U }, search.Results().Count());
        AssertRow(search, 2, 2U, L"0x0002", L"0x02", L"0x02");

        search.SetComparisonType(ComparisonType::GreaterThan);
        search.SetFilterValue(L"3");
        search.ApplyFilter();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"2/2"), search.GetSelectedPage());
        Assert::AreEqual(std::wstring(L"Filter: > 3"), search.GetFilterSummary());
        Assert::AreEqual({ 4U }, search.GetResultCount());
        Assert::AreEqual({ 4U }, search.Results().Count());
        AssertRow(search, 2, 6U, L"0x0006", L"0x06", L"0x06");

        search.SetComparisonType(ComparisonType::NotEqualTo);
        search.SetFilterValue(L"6");
        search.ApplyFilter();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"3/3"), search.GetSelectedPage());
        Assert::AreEqual(std::wstring(L"Filter: != 6"), search.GetFilterSummary());
        Assert::AreEqual({ 3U }, search.GetResultCount());
        Assert::AreEqual({ 3U }, search.Results().Count());
        AssertRow(search, 2, 7U, L"0x0007", L"0x07", L"0x07");

        // cannot go forward
        search.NextPage();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"3/3"), search.GetSelectedPage());
        Assert::AreEqual(std::wstring(L"Filter: != 6"), search.GetFilterSummary());
        Assert::AreEqual({ 3U }, search.GetResultCount());
        Assert::AreEqual({ 3U }, search.Results().Count());
        AssertRow(search, 2, 7U, L"0x0007", L"0x07", L"0x07");

        search.PreviousPage();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"2/3"), search.GetSelectedPage());
        Assert::AreEqual(std::wstring(L"Filter: > 3"), search.GetFilterSummary());
        Assert::AreEqual({ 4U }, search.GetResultCount());
        Assert::AreEqual({ 4U }, search.Results().Count());
        AssertRow(search, 2, 6U, L"0x0006", L"0x06", L"0x06");

        search.PreviousPage();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"1/3"), search.GetSelectedPage());
        Assert::AreEqual(std::wstring(L"Filter: < 8"), search.GetFilterSummary());
        Assert::AreEqual({ 8U }, search.GetResultCount());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual({ 8U }, search.Results().Count());
        AssertRow(search, 2, 2U, L"0x0002", L"0x02", L"0x02");

        // cannot go back any further
        search.PreviousPage();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"1/3"), search.GetSelectedPage());
        Assert::AreEqual(std::wstring(L"Filter: < 8"), search.GetFilterSummary());
        Assert::AreEqual({ 8U }, search.GetResultCount());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual({ 8U }, search.Results().Count());
        AssertRow(search, 2, 2U, L"0x0002", L"0x02", L"0x02");

        search.NextPage();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"2/3"), search.GetSelectedPage());
        Assert::AreEqual(std::wstring(L"Filter: > 3"), search.GetFilterSummary());
        Assert::AreEqual({ 4U }, search.GetResultCount());
        Assert::AreEqual({ 4U }, search.Results().Count());
        AssertRow(search, 2, 6U, L"0x0006", L"0x06", L"0x06");

        search.PreviousPage();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"1/3"), search.GetSelectedPage());
        Assert::AreEqual(std::wstring(L"Filter: < 8"), search.GetFilterSummary());
        Assert::AreEqual({ 8U }, search.GetResultCount());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual({ 8U }, search.Results().Count());
        AssertRow(search, 2, 2U, L"0x0002", L"0x02", L"0x02");

        // not at last page - filter should replace any pages after current
        search.SetComparisonType(ComparisonType::Equals);
        search.SetFilterValue(L"5");
        search.ApplyFilter();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"2/2"), search.GetSelectedPage());
        Assert::AreEqual(std::wstring(L"Filter: = 5"), search.GetFilterSummary());
        Assert::AreEqual({ 1U }, search.GetResultCount());
        Assert::AreEqual({ 1U }, search.Results().Count());
        AssertRow(search, 0, 5U, L"0x0005", L"0x05", L"0x05");

        search.PreviousPage();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"1/2"), search.GetSelectedPage());
        Assert::AreEqual(std::wstring(L"Filter: < 8"), search.GetFilterSummary());
        Assert::AreEqual({ 8U }, search.GetResultCount());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual({ 8U }, search.Results().Count());
        AssertRow(search, 2, 2U, L"0x0002", L"0x02", L"0x02");

        search.NextPage();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"2/2"), search.GetSelectedPage());
        Assert::AreEqual(std::wstring(L"Filter: = 5"), search.GetFilterSummary());
        Assert::AreEqual({ 1U }, search.GetResultCount());
        Assert::AreEqual({ 1U }, search.Results().Count());
        AssertRow(search, 0, 5U, L"0x0005", L"0x05", L"0x05");
    }

    TEST_METHOD(TestDoFramePreviousPage)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.BeginNewSearch();

        search.SetComparisonType(ComparisonType::LessThan);
        search.SetValueType(MemorySearchViewModel::ValueType::Constant);
        search.SetFilterValue(L"8");
        search.ApplyFilter();
        Assert::AreEqual({ 8U }, search.GetResultCount());

        search.SetComparisonType(ComparisonType::GreaterThan);
        search.SetFilterValue(L"3");
        search.ApplyFilter();
        Assert::AreEqual({ 4U }, search.GetResultCount());

        search.SetComparisonType(ComparisonType::NotEqualTo);
        search.SetFilterValue(L"6");
        search.ApplyFilter();
        Assert::AreEqual({ 3U }, search.GetResultCount());

        search.PreviousPage();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"2/3"), search.GetSelectedPage());
        Assert::AreEqual({ 4U }, search.GetResultCount());
        Assert::AreEqual({ 4U }, search.Results().Count());
        AssertRow(search, 2, 6U, L"0x0006", L"0x06", L"0x06");

        search.memory.at(6) = 9;
        search.DoFrame();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"2/3"), search.GetSelectedPage());
        Assert::AreEqual({ 4U }, search.GetResultCount());
        Assert::AreEqual({ 4U }, search.Results().Count());
        AssertRow(search, 2, 6U, L"0x0006", L"0x09", L"0x06");
    }

    TEST_METHOD(TestExcludeSelected)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.BeginNewSearch();

        search.SetComparisonType(ComparisonType::LessThan);
        search.SetValueType(MemorySearchViewModel::ValueType::Constant);
        search.SetFilterValue(L"5");

        search.ApplyFilter();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 5U }, search.GetResultCount());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());

        Assert::AreEqual({ 5U }, search.Results().Count());
        search.Results().GetItemAt(2)->SetSelected(true);
        search.Results().GetItemAt(3)->SetSelected(true);

        search.ExcludeSelected();
        Assert::AreEqual({ 3U }, search.Results().Count());
        Assert::AreEqual(0U, search.Results().GetItemAt(0)->nAddress);
        Assert::AreEqual(1U, search.Results().GetItemAt(1)->nAddress);
        Assert::AreEqual(4U, search.Results().GetItemAt(2)->nAddress);
        Assert::IsFalse(search.Results().GetItemAt(2)->IsSelected());

        search.ExcludeSelected();
        Assert::AreEqual({ 3U }, search.Results().Count());
    }

    TEST_METHOD(TestBookmarkSelected)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.BeginNewSearch();

        search.SetComparisonType(ComparisonType::LessThan);
        search.SetValueType(MemorySearchViewModel::ValueType::Constant);
        search.SetFilterValue(L"5");

        search.ApplyFilter();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 5U }, search.GetResultCount());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());

        Assert::AreEqual({ 5U }, search.Results().Count());
        search.Results().GetItemAt(2)->SetSelected(true);
        search.Results().GetItemAt(3)->SetSelected(true);

        search.BookmarkSelected();
        Assert::AreEqual({ 5U }, search.Results().Count());

        const auto& pBookmarks = search.mockWindowManager.MemoryBookmarks.Bookmarks();
        Assert::AreEqual({ 2U }, pBookmarks.Count());
        Assert::AreEqual({ 2U }, pBookmarks.GetItemAt(0)->GetAddress());
        Assert::AreEqual({ 3U }, pBookmarks.GetItemAt(1)->GetAddress());

        Assert::IsTrue(search.Results().GetItemAt(2)->IsSelected());
        Assert::IsTrue(search.Results().GetItemAt(3)->IsSelected());
    }

    TEST_METHOD(TestBookmarkSelectedFourBit)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.SetSearchType(MemorySearchViewModel::SearchType::FourBit);
        search.BeginNewSearch();

        search.SetComparisonType(ComparisonType::GreaterThan);
        search.SetValueType(MemorySearchViewModel::ValueType::Constant);
        search.SetFilterValue(L"13");

        search.ApplyFilter();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 4U }, search.GetResultCount());
        Assert::AreEqual(MemSize::Nibble_Lower, search.ResultMemSize());

        Assert::AreEqual({ 4U }, search.Results().Count());
        search.Results().GetItemAt(2)->SetSelected(true);
        search.Results().GetItemAt(3)->SetSelected(true);

        bool bSawDialog = false;
        search.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bSawDialog](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bSawDialog = true;
            Assert::AreEqual(std::wstring(L"4-bit bookmarks are not supported"), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::OK;
        });

        search.BookmarkSelected();
        Assert::IsTrue(bSawDialog);
        Assert::AreEqual({ 4U }, search.Results().Count());

        const auto& pBookmarks = search.mockWindowManager.MemoryBookmarks.Bookmarks();
        Assert::AreEqual({ 0U }, pBookmarks.Count());

        Assert::IsTrue(search.Results().GetItemAt(2)->IsSelected());
        Assert::IsTrue(search.Results().GetItemAt(3)->IsSelected());
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra