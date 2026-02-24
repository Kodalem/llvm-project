#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "SinglePath/MemoryAccessAnalysis.h"
#include "Mocks.h"

using namespace llvm;

using ::testing::Return;
using ::testing::Contains;
using ::testing::SizeIs;
using ::testing::UnorderedElementsAreArray;
using ::testing::Eq;
using ::testing::IsEmpty;

namespace {

typedef MockMBB<std::tuple<int,int,int>> MockMBB;
typedef MockLoop<std::tuple<int,int,int>> MockLoop;
typedef MockLoopInfo<std::tuple<int,int,int>> MockLoopInfo;

#define mockMBB(name, accesses, loopbound_min, loopbound_max) mockMBBBase(name, std::make_tuple(accesses,loopbound_min,loopbound_max))
#define pair(p1,p2) std::make_pair(p1,p2)
#define pair_u(p1,p2) std::make_pair((unsigned)p1,(unsigned)p2)

unsigned countAccess(const MockMBB *mbb){
    return std::get<0>(mbb->payload);
}

std::pair<uint64_t, uint64_t> loopbounds(const MockMBB *mbb){
  assert(std::get<1>(mbb->payload)>=1);
  assert(std::get<1>(mbb->payload)<=std::get<2>(mbb->payload));
  return pair(std::get<1>(mbb->payload),std::get<2>(mbb->payload));
}

TEST(MemoryAccessAnalysisTest, OneMBB){
  MockLoopInfo LI;

  mockMBB(mockMBB1,0,-1,-1);
  set_preds_succs(mockMBB1, arr({}),arr({}));

  auto result1 = memoryAccessAnalysis(&mockMBB1,&LI,countAccess,loopbounds);
  EXPECT_THAT(
    result1.first,
    UnorderedElementsAreArray({
      pair(&mockMBB1, pair(0,0)),
    })
  );
  EXPECT_THAT(result1.second,IsEmpty());

  mockMBB(mockMBB2,1,-1,-1);
  set_preds_succs(mockMBB2, arr({}), arr({}));

  auto result2 = memoryAccessAnalysis(&mockMBB2,&LI,countAccess,loopbounds);
  EXPECT_THAT(
    result2.first,
    UnorderedElementsAreArray({
      pair(&mockMBB2, pair(1,1)),
    })
  );
  EXPECT_THAT(result2.second,IsEmpty());

  mockMBB(mockMBB3,1345,-1,-1);
  set_preds_succs(mockMBB3, arr({}), arr({}));

  auto result3 = memoryAccessAnalysis(&mockMBB3,&LI,countAccess,loopbounds);
  EXPECT_THAT(
      result3.first,
      UnorderedElementsAreArray({
        pair(&mockMBB3, pair(1345,1345)),
      })
  );
  EXPECT_THAT(result3.second,IsEmpty());
}

TEST(MemoryAccessAnalysisTest, Consecutive){
  MockLoopInfo LI;

  mockMBB(mockMBB1,3,-1,-1);
  mockMBB(mockMBB2,4,-1,-1);
  set_preds_succs(mockMBB1, arr({}),arr({&mockMBB2}));
  set_preds_succs(mockMBB2, arr({&mockMBB1}),arr({}));

  auto result = memoryAccessAnalysis(&mockMBB1,&LI,countAccess,loopbounds);
  EXPECT_THAT(
      result.first,
      UnorderedElementsAreArray({
        pair(&mockMBB1, pair(3,3)),
        pair(&mockMBB2, pair(7,7)),
      })
  );
  EXPECT_THAT(result.second,IsEmpty());
}

TEST(MemoryAccessAnalysisTest, FirstCountZero){
  MockLoopInfo LI;

  mockMBB(mockMBB1,0,-1,-1);
  mockMBB(mockMBB2,6,-1,-1);
  set_preds_succs(mockMBB1, arr({}),arr({&mockMBB2}));
  set_preds_succs(mockMBB2, arr({&mockMBB1}),arr({}));

  auto result = memoryAccessAnalysis(&mockMBB1,&LI,countAccess,loopbounds);
  EXPECT_THAT(
      result.first,
      UnorderedElementsAreArray({
        pair(&mockMBB1, pair(0,0)),
        pair(&mockMBB2, pair(6,6)),
      })
  );
  EXPECT_THAT(result.second,IsEmpty());
}

TEST(MemoryAccessAnalysisTest, Branch){
  MockLoopInfo LI;

  mockMBB(mockMBB1,1,-1,-1);
  mockMBB(mockMBB2,2,-1,-1);
  mockMBB(mockMBB3,3,-1,-1);
  mockMBB(mockMBB4,4,-1,-1);
  set_preds_succs(mockMBB1, arr({}), arr({&mockMBB2, &mockMBB3}));
  set_preds_succs(mockMBB2, arr({&mockMBB1}),arr({&mockMBB4}));
  set_preds_succs(mockMBB3, arr({&mockMBB1}),arr({&mockMBB4}));
  set_preds_succs(mockMBB4, arr({&mockMBB2, &mockMBB3}), arr({}));

  auto result = memoryAccessAnalysis(&mockMBB1,&LI,countAccess,loopbounds);
  EXPECT_THAT(
      result.first,
      UnorderedElementsAreArray({
        pair(&mockMBB1, pair(1,1)),
        pair(&mockMBB2, pair(3,3)),
        pair(&mockMBB3, pair(4,4)),
        pair(&mockMBB4, pair(7,8)),
      })
  );
  EXPECT_THAT(result.second,IsEmpty());
}

TEST(MemoryAccessAnalysisTest, Loop){
  MockLoopInfo LI;

  mockMBB(mockMBB1,1,-1,-1);
  mockMBB(mockMBB2,2,1,4);
  mockMBB(mockMBB3,3,-1,-1);
  mockMBB(mockMBB4,4,-1,-1);
  set_preds_succs(mockMBB1, arr({}), arr({&mockMBB2}));
  set_preds_succs(mockMBB2, arr({&mockMBB1,&mockMBB3}),arr({&mockMBB3,&mockMBB4}));
  set_preds_succs(mockMBB3, arr({&mockMBB2}),arr({&mockMBB2}));
  set_preds_succs(mockMBB4, arr({&mockMBB2}), arr({}));

  MockLoop mockLoop1(nullptr,
      { &mockMBB2, &mockMBB3 },
      { &mockMBB3 },
      { &mockMBB2 }
  );
  LI.add_loop(&mockLoop1);

  auto result = memoryAccessAnalysis(&mockMBB1,&LI,countAccess,loopbounds);
  EXPECT_THAT(
      result.first,
      UnorderedElementsAreArray({
        pair(&mockMBB1, pair(1,1)),
        pair(&mockMBB2, pair(2,2)),
        pair(&mockMBB3, pair(5,5)),
        pair(&mockMBB4, pair(7,22)),
      })
  );
  EXPECT_THAT(result.second, SizeIs(1));
  EXPECT_THAT(std::get<0>(result.second[&mockMBB2]), Eq(pair_u(1,1)));
  EXPECT_THAT(std::get<1>(result.second[&mockMBB2]), Eq(pair_u(5,5)));
  EXPECT_THAT(
      std::get<2>(result.second[&mockMBB2]),
      UnorderedElementsAreArray({
        pair(&mockMBB2, pair(3,18)),
      })
  );
}

TEST(MemoryAccessAnalysisTest, LoopConst){
  MockLoopInfo LI;

  mockMBB(mockMBB1,1,-1,-1);
  mockMBB(mockMBB2,2,5,5);
  mockMBB(mockMBB3,3,-1,-1);
  mockMBB(mockMBB4,4,-1,-1);
  set_preds_succs(mockMBB1, arr({}), arr({&mockMBB2}));
  set_preds_succs(mockMBB2, arr({&mockMBB1,&mockMBB3}),arr({&mockMBB3,&mockMBB4}));
  set_preds_succs(mockMBB3, arr({&mockMBB2}),arr({&mockMBB2}));
  set_preds_succs(mockMBB4, arr({&mockMBB2}), arr({}));

  MockLoop mockLoop1(nullptr,
      { &mockMBB2, &mockMBB3 },
      { &mockMBB3 },
      { &mockMBB2 }
  );
  LI.add_loop(&mockLoop1);

  auto result = memoryAccessAnalysis(&mockMBB1,&LI,countAccess,loopbounds);
  EXPECT_THAT(
      result.first,
      UnorderedElementsAreArray({
        pair(&mockMBB1, pair(1,1)),
        pair(&mockMBB2, pair(2,2)),
        pair(&mockMBB3, pair(5,5)),
        pair(&mockMBB4, pair(27,27)),
      })
  );
  EXPECT_THAT(result.second, SizeIs(1));
  EXPECT_THAT(std::get<0>(result.second[&mockMBB2]), Eq(pair_u(1,1)));
  EXPECT_THAT(std::get<1>(result.second[&mockMBB2]), Eq(pair_u(5,5)));
  EXPECT_THAT(
      std::get<2>(result.second[&mockMBB2]),
      UnorderedElementsAreArray({
        pair(&mockMBB2, pair(23,23)),
      })
  );
}

TEST(MemoryAccessAnalysisTest, LoopZeroHeader){
  MockLoopInfo LI;

  mockMBB(mockMBB1,1,-1,-1);
  mockMBB(mockMBB2,0,1,4);
  mockMBB(mockMBB3,3,-1,-1);
  mockMBB(mockMBB4,4,-1,-1);
  set_preds_succs(mockMBB1, arr({}), arr({&mockMBB2}));
  set_preds_succs(mockMBB2, arr({&mockMBB1,&mockMBB3}),arr({&mockMBB3,&mockMBB4}));
  set_preds_succs(mockMBB3, arr({&mockMBB2}),arr({&mockMBB2}));
  set_preds_succs(mockMBB4, arr({&mockMBB2}), arr({}));

  MockLoop mockLoop1(nullptr,
      { &mockMBB2, &mockMBB3 },
      { &mockMBB3 },
      { &mockMBB2 }
  );
  LI.add_loop(&mockLoop1);

  auto result = memoryAccessAnalysis(&mockMBB1,&LI,countAccess,loopbounds);
  EXPECT_THAT(
      result.first,
      UnorderedElementsAreArray({
        pair(&mockMBB1, pair(1,1)),
        pair(&mockMBB2, pair(0,0)),
        pair(&mockMBB3, pair(3,3)),
        pair(&mockMBB4, pair(5,14)),
      })
  );
  EXPECT_THAT(result.second, SizeIs(1));
  EXPECT_THAT(std::get<0>(result.second[&mockMBB2]), Eq(pair_u(1,1)));
  EXPECT_THAT(std::get<1>(result.second[&mockMBB2]), Eq(pair_u(3,3)));
  EXPECT_THAT(
      std::get<2>(result.second[&mockMBB2]),
      UnorderedElementsAreArray({
        pair(&mockMBB2, pair(1,10)),
      })
  );
}

TEST(MemoryAccessAnalysisTest, LoopOneMBB){
  MockLoopInfo LI;

  mockMBB(mockMBB1,1,-1,-1);
  mockMBB(mockMBB2,2,3,8);
  mockMBB(mockMBB3,3,-1,-1);
  set_preds_succs(mockMBB1, arr({}), arr({&mockMBB2}));
  set_preds_succs(mockMBB2, arr({&mockMBB1,&mockMBB2}),arr({&mockMBB2,&mockMBB3}));
  set_preds_succs(mockMBB3, arr({&mockMBB2}),arr({}));

  MockLoop mockLoop1(nullptr,
      { &mockMBB2 },
      { &mockMBB2 },
      { &mockMBB2 }
  );
  LI.add_loop(&mockLoop1);

  auto result = memoryAccessAnalysis(&mockMBB1,&LI,countAccess,loopbounds);
  EXPECT_THAT(
      result.first,
      UnorderedElementsAreArray({
        pair(&mockMBB1, pair(1,1)),
        pair(&mockMBB2, pair(2,2)),
        pair(&mockMBB3, pair(10,20)),
      })
  );
  EXPECT_THAT(result.second, SizeIs(1));
  EXPECT_THAT(std::get<0>(result.second[&mockMBB2]), Eq(pair_u(1,1)));
  EXPECT_THAT(std::get<1>(result.second[&mockMBB2]), Eq(pair_u(2,2)));
  EXPECT_THAT(
      std::get<2>(result.second[&mockMBB2]),
      UnorderedElementsAreArray({
        pair(&mockMBB2, pair(7,17)),
      })
  );
}

TEST(MemoryAccessAnalysisTest, LoopMultiPreds){
  MockLoopInfo LI;

  mockMBB(mockMBB1,1,-1,-1);
  mockMBB(mockMBB2,2,-1,-1);
  mockMBB(mockMBB3,3,-1,-1);
  mockMBB(mockMBB4,4,3,8);
  mockMBB(mockMBB5,5,-1,-1);
  set_preds_succs(mockMBB1, arr({}), arr({&mockMBB2,&mockMBB3}));
  set_preds_succs(mockMBB2, arr({&mockMBB1}), arr({&mockMBB4}));
  set_preds_succs(mockMBB3, arr({&mockMBB1}), arr({&mockMBB4}));
  set_preds_succs(mockMBB4, arr({&mockMBB2,&mockMBB3,&mockMBB4}),arr({&mockMBB4,&mockMBB5}));
  set_preds_succs(mockMBB5, arr({&mockMBB4}),arr({}));

  MockLoop mockLoop1(nullptr,
      { &mockMBB4 },
      { &mockMBB4 },
      { &mockMBB4 }
  );
  LI.add_loop(&mockLoop1);

  auto result = memoryAccessAnalysis(&mockMBB1,&LI,countAccess,loopbounds);
  EXPECT_THAT(
      result.first,
      UnorderedElementsAreArray({
        pair(&mockMBB1, pair(1,1)),
        pair(&mockMBB2, pair(3,3)),
        pair(&mockMBB3, pair(4,4)),
        pair(&mockMBB4, pair(4,4)),
        pair(&mockMBB5, pair(20,41)),
      })
  );
  EXPECT_THAT(result.second, SizeIs(1));
  EXPECT_THAT(std::get<0>(result.second[&mockMBB4]), Eq(pair_u(3,4)));
  EXPECT_THAT(std::get<1>(result.second[&mockMBB4]), Eq(pair_u(4,4)));
  EXPECT_THAT(
      std::get<2>(result.second[&mockMBB4]),
      UnorderedElementsAreArray({
        pair(&mockMBB4, pair(15,36)),
      })
  );
}

TEST(MemoryAccessAnalysisTest, LoopNonHeaderExit){
  MockLoopInfo LI;

  mockMBB(mockMBB1,1,-1,-1);
  mockMBB(mockMBB2,2,1,2);
  mockMBB(mockMBB3,3,-1,-1);
  mockMBB(mockMBB4,4,-1,-1);
  set_preds_succs(mockMBB1, arr({}), arr({&mockMBB2}));
  set_preds_succs(mockMBB2, arr({&mockMBB1}),arr({&mockMBB3}));
  set_preds_succs(mockMBB3, arr({&mockMBB2}),arr({&mockMBB2,&mockMBB4}));
  set_preds_succs(mockMBB4, arr({&mockMBB3}), arr({}));
  MockLoop mockLoop1(nullptr,
      { &mockMBB2, &mockMBB3 },
      { &mockMBB3 },
      { &mockMBB3 }
  );
  LI.add_loop(&mockLoop1);

  auto result = memoryAccessAnalysis(&mockMBB1,&LI,countAccess,loopbounds);
  EXPECT_THAT(
      result.first,
      UnorderedElementsAreArray({
        pair(&mockMBB1, pair(1,1)),
        pair(&mockMBB2, pair(2,2)),
        pair(&mockMBB3, pair(5,5)),
        pair(&mockMBB4, pair(10,15)),
      })
  );
  EXPECT_THAT(result.second, SizeIs(1));
  EXPECT_THAT(std::get<0>(result.second[&mockMBB2]), Eq(pair_u(1,1)));
  EXPECT_THAT(std::get<1>(result.second[&mockMBB2]), Eq(pair_u(5,5)));
  EXPECT_THAT(
      std::get<2>(result.second[&mockMBB2]),
      UnorderedElementsAreArray({
        pair(&mockMBB3, pair(6,11)),
      })
  );
}

TEST(MemoryAccessAnalysisTest, LoopMultipleExits){
  MockLoopInfo LI;

  mockMBB(mockMBB1,1,-1,-1);
  mockMBB(mockMBB2,2,4,17);
  mockMBB(mockMBB3,3,-1,-1);
  mockMBB(mockMBB4,4,-1,-1);
  set_preds_succs(mockMBB1, arr({}), arr({&mockMBB2}));
  set_preds_succs(mockMBB2, arr({&mockMBB1}),arr({&mockMBB3,&mockMBB4}));
  set_preds_succs(mockMBB3, arr({&mockMBB2}),arr({&mockMBB2,&mockMBB4}));
  set_preds_succs(mockMBB4, arr({&mockMBB2,&mockMBB3}), arr({}));
  MockLoop mockLoop1(nullptr,
      { &mockMBB2, &mockMBB3 },
      { &mockMBB3 },
      { &mockMBB2,&mockMBB3 }
  );
  LI.add_loop(&mockLoop1);

  auto result = memoryAccessAnalysis(&mockMBB1,&LI,countAccess,loopbounds);
  EXPECT_THAT(
      result.first,
      UnorderedElementsAreArray({
        pair(&mockMBB1, pair(1,1)),
        pair(&mockMBB2, pair(2,2)),
        pair(&mockMBB3, pair(5,5)),
        pair(&mockMBB4, pair(22,90)),
      })
  );
  EXPECT_THAT(result.second, SizeIs(1));
  EXPECT_THAT(std::get<0>(result.second[&mockMBB2]), Eq(pair_u(1,1)));
  EXPECT_THAT(std::get<1>(result.second[&mockMBB2]), Eq(pair_u(5,5)));
  EXPECT_THAT(
      std::get<2>(result.second[&mockMBB2]),
      UnorderedElementsAreArray({
        pair(&mockMBB2, pair(18,83)),
        pair(&mockMBB3, pair(21,86)),
      })
  );
}

TEST(MemoryAccessAnalysisTest, LoopNonHeaderNonLatchExit){
  MockLoopInfo LI;

  mockMBB(mockMBB1,0,-1,-1);
  mockMBB(mockMBB2,2,1,3);
  mockMBB(mockMBB3,0,-1,-1);
  mockMBB(mockMBB4,4,-1,-1);
  mockMBB(mockMBB5,2,-1,-1);
  set_preds_succs(mockMBB1, arr({}), arr({&mockMBB2}));
  set_preds_succs(mockMBB2, arr({&mockMBB1}),arr({&mockMBB3}));
  set_preds_succs(mockMBB3, arr({&mockMBB2}),arr({&mockMBB4,&mockMBB5}));
  set_preds_succs(mockMBB4, arr({&mockMBB3}), arr({&mockMBB2}));
  set_preds_succs(mockMBB5, arr({&mockMBB3}), arr({}));
  MockLoop mockLoop1(nullptr,
      { &mockMBB2, &mockMBB3, &mockMBB4 },
      { &mockMBB4 },
      { &mockMBB3 }
  );
  LI.add_loop(&mockLoop1);

  auto result = memoryAccessAnalysis(&mockMBB1,&LI,countAccess,loopbounds);
  EXPECT_THAT(
      result.first,
      UnorderedElementsAreArray({
        pair(&mockMBB1, pair(0,0)),
        pair(&mockMBB2, pair(2,2)),
        pair(&mockMBB3, pair(2,2)),
        pair(&mockMBB4, pair(6,6)),
        pair(&mockMBB5, pair(4,16)),
      })
  );
  EXPECT_THAT(result.second, SizeIs(1));
  EXPECT_THAT(std::get<0>(result.second[&mockMBB2]), Eq(pair_u(0,0)));
  EXPECT_THAT(std::get<1>(result.second[&mockMBB2]), Eq(pair_u(6,6)));
  EXPECT_THAT(
      std::get<2>(result.second[&mockMBB2]),
      UnorderedElementsAreArray({
        pair(&mockMBB3, pair(2,14)),
      })
  );
}

TEST(MemoryAccessAnalysisTest, LoopMultipleLatches){
  MockLoopInfo LI;

  mockMBB(mockMBB1,1,-1,-1);
  mockMBB(mockMBB2,2,2,7);
  mockMBB(mockMBB3,3,-1,-1);
  mockMBB(mockMBB4,4,-1,-1);
  mockMBB(mockMBB5,5,-1,-1);
  set_preds_succs(mockMBB1, arr({}), arr({&mockMBB2}));
  set_preds_succs(mockMBB2, arr({&mockMBB1,&mockMBB3,&mockMBB4}),arr({&mockMBB3,&mockMBB5}));
  set_preds_succs(mockMBB3, arr({&mockMBB2}),arr({&mockMBB4,&mockMBB2}));
  set_preds_succs(mockMBB4, arr({&mockMBB3}),arr({&mockMBB2}));
  set_preds_succs(mockMBB5, arr({&mockMBB2}), arr({}));

  MockLoop mockLoop1(nullptr,
      { &mockMBB2, &mockMBB3, &mockMBB4 },
      { &mockMBB3, &mockMBB4 },
      { &mockMBB2 }
  );
  LI.add_loop(&mockLoop1);

  auto result = memoryAccessAnalysis(&mockMBB1,&LI,countAccess,loopbounds);
  EXPECT_THAT(
      result.first,
      UnorderedElementsAreArray({
        pair(&mockMBB1, pair(1,1)),
        pair(&mockMBB2, pair(2,2)),
        pair(&mockMBB3, pair(5,5)),
        pair(&mockMBB4, pair(9,9)),
        pair(&mockMBB5, pair(13,62)),
      })
  );
  EXPECT_THAT(result.second, SizeIs(1));
  EXPECT_THAT(std::get<0>(result.second[&mockMBB2]), Eq(pair_u(1,1)));
  EXPECT_THAT(std::get<1>(result.second[&mockMBB2]), Eq(pair_u(5,9)));
  EXPECT_THAT(
      std::get<2>(result.second[&mockMBB2]),
      UnorderedElementsAreArray({
        pair(&mockMBB2, pair(8,57)),
      })
  );
}

TEST(MemoryAccessAnalysisTest, LoopNested){
  MockLoopInfo LI;

  mockMBB(mockMBB1,10,-1,-1);
  mockMBB(mockMBB2,20,4,7);
  mockMBB(mockMBB3,30,2,3);
  mockMBB(mockMBB4,40,-1,-1);
  mockMBB(mockMBB5,50,-1,-1);
  set_preds_succs(mockMBB1, arr({}), arr({&mockMBB2}));
  set_preds_succs(mockMBB2, arr({&mockMBB1,&mockMBB3}),arr({&mockMBB3,&mockMBB5}));
  set_preds_succs(mockMBB3, arr({&mockMBB2,&mockMBB4}),arr({&mockMBB2,&mockMBB4}));
  set_preds_succs(mockMBB4, arr({&mockMBB3}),arr({&mockMBB3}));
  set_preds_succs(mockMBB5, arr({&mockMBB2}), arr({}));

  MockLoop mockLoop1(nullptr,
      { &mockMBB2, &mockMBB3, &mockMBB4 },
      { &mockMBB3 },
      { &mockMBB2 }
  );
  MockLoop mockLoop2(&mockLoop1,
      { &mockMBB3, &mockMBB4 },
      { &mockMBB4 },
      { &mockMBB3 }
  );
  LI.add_loop(&mockLoop1);
  LI.add_loop(&mockLoop2);

  auto result = memoryAccessAnalysis(&mockMBB1,&LI,countAccess,loopbounds);
  EXPECT_THAT(
      result.first,
      UnorderedElementsAreArray({
        pair(&mockMBB1, pair(10,10)),
        pair(&mockMBB2, pair(20,20)),
        pair(&mockMBB3, pair(30,30)),
        pair(&mockMBB4, pair(70,70)),
        pair(&mockMBB5, pair(440,1220)),
      })
  );
  EXPECT_THAT(result.second, SizeIs(2));
  EXPECT_THAT(std::get<0>(result.second[&mockMBB2]), Eq(pair_u(10,10)));
  EXPECT_THAT(std::get<1>(result.second[&mockMBB2]), Eq(pair_u(120,190)));
  EXPECT_THAT(
      std::get<2>(result.second[&mockMBB2]),
      UnorderedElementsAreArray({
        pair(&mockMBB2, pair(390,1170)),
      })
  );
  EXPECT_THAT(std::get<0>(result.second[&mockMBB3]), Eq(pair_u(20,20)));
  EXPECT_THAT(std::get<1>(result.second[&mockMBB3]), Eq(pair_u(70,70)));
  EXPECT_THAT(
      std::get<2>(result.second[&mockMBB3]),
      UnorderedElementsAreArray({
        pair(&mockMBB3, pair(120,190)),
      })
  );
}

TEST(MemoryAccessAnalysisTest, LoopLatchFromNested){
  MockLoopInfo LI;

  mockMBB(mockMBB1,11,-1,-1);
  mockMBB(mockMBB2,22,1,2);
  mockMBB(mockMBB3,33,1,4);
  mockMBB(mockMBB4,44,-1,-1);
  mockMBB(mockMBB5,55,-1,-1);
  set_preds_succs(mockMBB1, arr({}), arr({&mockMBB2}));
  set_preds_succs(mockMBB2, arr({&mockMBB1,&mockMBB3,&mockMBB4}),arr({&mockMBB3,&mockMBB5}));
  set_preds_succs(mockMBB3, arr({&mockMBB2,&mockMBB4}),arr({&mockMBB2,&mockMBB4}));
  set_preds_succs(mockMBB4, arr({&mockMBB3}),arr({&mockMBB3,&mockMBB2}));
  set_preds_succs(mockMBB5, arr({&mockMBB2}), arr({}));

  MockLoop mockLoop1(nullptr,
      {&mockMBB2,&mockMBB3,&mockMBB4},
      {&mockMBB3,&mockMBB4},
      {&mockMBB2}
  );
  MockLoop mockLoop2(&mockLoop1,
      {&mockMBB3,&mockMBB4},
      {&mockMBB4},
      {&mockMBB3,&mockMBB4}
  );
  LI.add_loop(&mockLoop1);
  LI.add_loop(&mockLoop2);

  auto result = memoryAccessAnalysis(&mockMBB1,&LI,countAccess,loopbounds);
  EXPECT_THAT(
      result.first,
      UnorderedElementsAreArray({
        pair(&mockMBB1, pair(11,11)),
        pair(&mockMBB2, pair(22,22)),
        pair(&mockMBB3, pair(33,33)),
        pair(&mockMBB4, pair(77,77)),
        pair(&mockMBB5, pair(88,418)),
      })
  );
  EXPECT_THAT(result.second, SizeIs(2));
  EXPECT_THAT(std::get<0>(result.second[&mockMBB2]), Eq(pair_u(11,11)));
  EXPECT_THAT(std::get<1>(result.second[&mockMBB2]), Eq(pair_u(55,330)));
  EXPECT_THAT(
      std::get<2>(result.second[&mockMBB2]),
      UnorderedElementsAreArray({
        pair(&mockMBB2, pair(33,363)),
      })
  );
  EXPECT_THAT(std::get<0>(result.second[&mockMBB3]), Eq(pair_u(22,22)));
  EXPECT_THAT(std::get<1>(result.second[&mockMBB3]), Eq(pair_u(77,77)));
  EXPECT_THAT(
      std::get<2>(result.second[&mockMBB3]),
      UnorderedElementsAreArray({
        pair(&mockMBB3, pair(55,286)),
        pair(&mockMBB4, pair(99,330)),
      })
  );
}

TEST(MemoryAccessAnalysisTest, LoopExitFromNested){
  MockLoopInfo LI;

  mockMBB(mockMBB1,12,-1,-1);
  mockMBB(mockMBB2,23,5,16);
  mockMBB(mockMBB3,34,3,6);
  mockMBB(mockMBB4,45,-1,-1);
  mockMBB(mockMBB5,56,-1,-1);
  set_preds_succs(mockMBB1, arr({}), arr({&mockMBB2}));
  set_preds_succs(mockMBB2, arr({&mockMBB1,&mockMBB3}),arr({&mockMBB3,&mockMBB5}));
  set_preds_succs(mockMBB3, arr({&mockMBB2,&mockMBB4}),arr({&mockMBB2,&mockMBB4}));
  set_preds_succs(mockMBB4, arr({&mockMBB3}),arr({&mockMBB3,&mockMBB5}));
  set_preds_succs(mockMBB5, arr({&mockMBB2,&mockMBB4}), arr({}));

  MockLoop mockLoop1(nullptr,
      { &mockMBB2, &mockMBB3, &mockMBB4 },
      { &mockMBB3 },
      { &mockMBB2, &mockMBB4 }
  );
  MockLoop mockLoop2(&mockLoop1,
      { &mockMBB3, &mockMBB4 },
      { &mockMBB4 },
      { &mockMBB3, &mockMBB4 }
  );
  LI.add_loop(&mockLoop1);
  LI.add_loop(&mockLoop2);

  auto result = memoryAccessAnalysis(&mockMBB1,&LI,countAccess,loopbounds);
  EXPECT_THAT(
      result.first,
      UnorderedElementsAreArray({
        pair(&mockMBB1, pair(12,12)),
        pair(&mockMBB2, pair(23,23)),
        pair(&mockMBB3, pair(34,34)),
        pair(&mockMBB4, pair(79,79)),
        pair(&mockMBB5, pair(951,7345)),
      })
  );
  EXPECT_THAT(result.second, SizeIs(2));
  EXPECT_THAT(std::get<0>(result.second[&mockMBB2]), Eq(pair_u(12,12)));
  EXPECT_THAT(std::get<1>(result.second[&mockMBB2]), Eq(pair_u(215,452)));
  EXPECT_THAT(
      std::get<2>(result.second[&mockMBB2]),
      UnorderedElementsAreArray({
        pair(&mockMBB2, pair(895,6815)),
        pair(&mockMBB4, pair(1132,7289)),
      })
  );
  EXPECT_THAT(std::get<0>(result.second[&mockMBB3]), Eq(pair_u(23,23)));
  EXPECT_THAT(std::get<1>(result.second[&mockMBB3]), Eq(pair_u(79,79)));
  EXPECT_THAT(
      std::get<2>(result.second[&mockMBB3]),
      UnorderedElementsAreArray({
        pair(&mockMBB3, pair(215,452)),
        pair(&mockMBB4, pair(260,497)),
      })
  );
}

TEST(MemoryAccessCompensationTest, OneMBB){
  MockLoopInfo LI;

  mockMBB(mockMBB1,9,-1,-1);
  set_preds_succs(mockMBB1, arr({}),arr({}));

  auto result = memoryAccessCompensation(&mockMBB1, &LI, countAccess);

  EXPECT_THAT(result,IsEmpty());
}

TEST(MemoryAccessCompensationTest, TwoMBB){
  MockLoopInfo LI;

  mockMBB(mockMBB1,9,-1,-1);
  mockMBB(mockMBB2,4,-1,-1);

  set_preds_succs(mockMBB1, arr({}),arr({&mockMBB2}));
  set_preds_succs(mockMBB2, arr({&mockMBB1}),arr({}));

  auto result = memoryAccessCompensation(&mockMBB1, &LI, countAccess);

  EXPECT_THAT(
      result,
      UnorderedElementsAreArray({
        pair(pair(&mockMBB1,&mockMBB2), 13),
      })
  );
}

TEST(MemoryAccessCompensationTest, FirstCountZero){
  MockLoopInfo LI;

  mockMBB(mockMBB1,0,-1,-1);
  mockMBB(mockMBB2,4,-1,-1);

  set_preds_succs(mockMBB1, arr({}),arr({&mockMBB2}));
  set_preds_succs(mockMBB2, arr({&mockMBB1}),arr({}));

  auto result = memoryAccessCompensation(&mockMBB1, &LI, countAccess);

  EXPECT_THAT(
      result,
      UnorderedElementsAreArray({
        pair(pair(&mockMBB1,&mockMBB2), 4),
      })
  );
}

TEST(MemoryAccessCompensationTest, AllZero){
  MockLoopInfo LI;

  mockMBB(mockMBB1,0,-1,-1);
  mockMBB(mockMBB2,0,-1,-1);

  set_preds_succs(mockMBB1, arr({}),arr({&mockMBB2}));
  set_preds_succs(mockMBB2, arr({&mockMBB1}),arr({}));

  auto result = memoryAccessCompensation(&mockMBB1, &LI, countAccess);

  EXPECT_THAT(result, IsEmpty());
}

TEST(MemoryAccessCompensationTest, TwoReturns){
  MockLoopInfo LI;

  mockMBB(mockMBB1,3,-1,-1);
  mockMBB(mockMBB2,24,-1,-1);
  mockMBB(mockMBB3,5,-1,-1);

  set_preds_succs(mockMBB1, arr({}),arr({&mockMBB2, &mockMBB3}));
  set_preds_succs(mockMBB2, arr({&mockMBB1}),arr({}));
  set_preds_succs(mockMBB3, arr({&mockMBB1}),arr({}));

  auto result = memoryAccessCompensation(&mockMBB1, &LI, countAccess);

  EXPECT_THAT(
      result,
      UnorderedElementsAreArray({
        pair(pair(&mockMBB1,&mockMBB2), 27),
        pair(pair(&mockMBB1,&mockMBB3), 8),
      })
  );
}

TEST(MemoryAccessCompensationTest, Merge){
  MockLoopInfo LI;

  mockMBB(mockMBB1,67,-1,-1);
  mockMBB(mockMBB2,2,-1,-1);
  mockMBB(mockMBB3,12,-1,-1);
  mockMBB(mockMBB4,8,-1,-1);

  set_preds_succs(mockMBB1, arr({}),arr({&mockMBB2, &mockMBB3}));
  set_preds_succs(mockMBB2, arr({&mockMBB1}),arr({&mockMBB4}));
  set_preds_succs(mockMBB3, arr({&mockMBB1}),arr({&mockMBB4}));
  set_preds_succs(mockMBB4, arr({&mockMBB2,&mockMBB3}),arr({}));

  auto result = memoryAccessCompensation(&mockMBB1, &LI, countAccess);

  EXPECT_THAT(
      result,
      UnorderedElementsAreArray({
        pair(pair(&mockMBB2,&mockMBB4), 77),
        pair(pair(&mockMBB3,&mockMBB4), 87),
      })
  );
}

TEST(MemoryAccessCompensationTest, MergeTwoReturns){
  MockLoopInfo LI;

  mockMBB(mockMBB1,14,-1,-1);
  mockMBB(mockMBB2,25,-1,-1);
  mockMBB(mockMBB3,0,-1,-1);
  mockMBB(mockMBB4,23,-1,-1);
  mockMBB(mockMBB5,2,-1,-1);
  mockMBB(mockMBB6,4,-1,-1);
  mockMBB(mockMBB7,16,-1,-1);

  set_preds_succs(mockMBB1, arr({}),arr({&mockMBB2, &mockMBB3, &mockMBB4, &mockMBB5}));
  set_preds_succs(mockMBB2, arr({&mockMBB1}),arr({&mockMBB6}));
  set_preds_succs(mockMBB3, arr({&mockMBB1}),arr({&mockMBB6}));
  set_preds_succs(mockMBB4, arr({&mockMBB1}),arr({&mockMBB7}));
  set_preds_succs(mockMBB5, arr({&mockMBB1}),arr({&mockMBB7}));
  set_preds_succs(mockMBB6, arr({&mockMBB2,&mockMBB3}),arr({}));
  set_preds_succs(mockMBB7, arr({&mockMBB4,&mockMBB5}),arr({}));

  auto result = memoryAccessCompensation(&mockMBB1, &LI, countAccess);

  EXPECT_THAT(
      result,
      UnorderedElementsAreArray({
        pair(pair(&mockMBB2,&mockMBB6), 43),
        pair(pair(&mockMBB3,&mockMBB6), 18),
        pair(pair(&mockMBB4,&mockMBB7), 53),
        pair(pair(&mockMBB5,&mockMBB7), 32),
      })
  );
}

TEST(MemoryAccessCompensationTest, TwoPredecessorsToNonEnd){
  MockLoopInfo LI;

  mockMBB(mockMBB1,2,-1,-1);
  mockMBB(mockMBB2,3,-1,-1);
  mockMBB(mockMBB3,4,-1,-1);
  mockMBB(mockMBB4,5,-1,-1);
  mockMBB(mockMBB5,6,-1,-1);

  set_preds_succs(mockMBB1, arr({}),arr({&mockMBB2,&mockMBB3}));
  set_preds_succs(mockMBB2, arr({&mockMBB1}),arr({&mockMBB4}));
  set_preds_succs(mockMBB3, arr({&mockMBB1}),arr({&mockMBB4}));
  set_preds_succs(mockMBB4, arr({&mockMBB2,&mockMBB3}),arr({&mockMBB5}));
  set_preds_succs(mockMBB5, arr({&mockMBB4}),arr({}));

  auto result = memoryAccessCompensation(&mockMBB1, &LI, countAccess);

  EXPECT_THAT(
      result,
      UnorderedElementsAreArray({
        pair(pair(&mockMBB2,&mockMBB4), 5),
        pair(pair(&mockMBB3,&mockMBB4), 6),
        pair(pair(&mockMBB4,&mockMBB5), 11),
      })
  );
}

TEST(MemoryAccessCompensationTest, TwoPredecessorToOptional){
  MockLoopInfo LI;

  mockMBB(mockMBB1,0,-1,-1);
  mockMBB(mockMBB2,1,-1,-1);
  mockMBB(mockMBB3,1,-1,-1);
  mockMBB(mockMBB4,0,-1,-1);
  set_preds_succs(mockMBB1, arr({}), arr({&mockMBB2, &mockMBB3}));
  set_preds_succs(mockMBB2, arr({&mockMBB1}),arr({&mockMBB3,&mockMBB4}));
  set_preds_succs(mockMBB3, arr({&mockMBB1,&mockMBB2}),arr({&mockMBB4}));
  set_preds_succs(mockMBB4, arr({&mockMBB2,&mockMBB3}),arr({}));

  auto result = memoryAccessCompensation(&mockMBB1, &LI, countAccess);

  EXPECT_THAT(
      result,
      UnorderedElementsAreArray({
        pair(pair(&mockMBB2,&mockMBB3), 1),
        pair(pair(&mockMBB2,&mockMBB4), 1),
        pair(pair(&mockMBB3,&mockMBB4), 1),
      })
  );
}

TEST(MemoryAccessCompensationTest, OneMBBLoop){
  MockLoopInfo LI;

  mockMBB(mockMBB1,54,-1,-1);
  mockMBB(mockMBB2,4,1,2);
  mockMBB(mockMBB3,7,-1,-1);

  set_preds_succs(mockMBB1, arr({}),arr({&mockMBB2}));
  set_preds_succs(mockMBB2, arr({&mockMBB1,&mockMBB2}),arr({&mockMBB2,&mockMBB3}));
  set_preds_succs(mockMBB3, arr({&mockMBB2}),arr({}));

  MockLoop mockLoop1(nullptr,
      { &mockMBB2 },
      { &mockMBB2 },
      { &mockMBB2 }
  );
  LI.add_loop(&mockLoop1);

  auto result = memoryAccessCompensation(&mockMBB1, &LI, countAccess);

  EXPECT_THAT(
      result,
      UnorderedElementsAreArray({
        pair(pair(&mockMBB2,&mockMBB2), 4),
        pair(pair(&mockMBB2,&mockMBB3), 65),
      })
  );
}

TEST(MemoryAccessCompensationTest, Loop){
  MockLoopInfo LI;

  mockMBB(mockMBB1,4,-1,-1);
  mockMBB(mockMBB2,4,1,9);
  mockMBB(mockMBB3,7,-1,-1);
  mockMBB(mockMBB4,25,-1,-1);

  set_preds_succs(mockMBB1, arr({}),arr({&mockMBB2}));
  set_preds_succs(mockMBB2, arr({&mockMBB1,&mockMBB3}),arr({&mockMBB3,&mockMBB4}));
  set_preds_succs(mockMBB3, arr({&mockMBB2}),arr({&mockMBB2}));
  set_preds_succs(mockMBB4, arr({&mockMBB2}),arr({}));

  MockLoop mockLoop1(nullptr,
      { &mockMBB2, &mockMBB3 },
      { &mockMBB3 },
      { &mockMBB2 }
  );
  LI.add_loop(&mockLoop1);

  auto result = memoryAccessCompensation(&mockMBB1, &LI, countAccess);

  EXPECT_THAT(
      result,
      UnorderedElementsAreArray({
        pair(pair(&mockMBB3,&mockMBB2), 11),
        pair(pair(&mockMBB2,&mockMBB4), 33),
      })
  );
}

TEST(MemoryAccessCompensationTest, LoopTwoLatches){
  MockLoopInfo LI;

  mockMBB(mockMBB1,23,-1,-1);
  mockMBB(mockMBB2,78,1,9);
  mockMBB(mockMBB3,45,-1,-1);
  mockMBB(mockMBB4,2,-1,-1);
  mockMBB(mockMBB5,5,-1,-1);

  set_preds_succs(mockMBB1, arr({}),arr({&mockMBB2}));
  set_preds_succs(mockMBB2, arr({&mockMBB1,&mockMBB3,&mockMBB4}),arr({&mockMBB3,&mockMBB5}));
  set_preds_succs(mockMBB3, arr({&mockMBB2}),arr({&mockMBB2,&mockMBB4}));
  set_preds_succs(mockMBB4, arr({&mockMBB3}),arr({&mockMBB2}));
  set_preds_succs(mockMBB5, arr({&mockMBB2}),arr({}));

  MockLoop mockLoop1(nullptr,
      { &mockMBB2, &mockMBB3, &mockMBB4 },
      { &mockMBB3,&mockMBB4 },
      { &mockMBB2 }
  );
  LI.add_loop(&mockLoop1);

  auto result = memoryAccessCompensation(&mockMBB1, &LI, countAccess);

  EXPECT_THAT(
      result,
      UnorderedElementsAreArray({
        pair(pair(&mockMBB3,&mockMBB2), 123),
        pair(pair(&mockMBB4,&mockMBB2), 125),
        pair(pair(&mockMBB2,&mockMBB5), 106),
      })
  );
}

TEST(MemoryAccessCompensationTest, LoopNonHeadExit){
  MockLoopInfo LI;

  mockMBB(mockMBB1,6,-1,-1);
  mockMBB(mockMBB2,9,1,11);
  mockMBB(mockMBB3,78,-1,-1);
  mockMBB(mockMBB4,34,-1,-1);

  set_preds_succs(mockMBB1, arr({}),arr({&mockMBB2}));
  set_preds_succs(mockMBB2, arr({&mockMBB1,&mockMBB3}),arr({&mockMBB3}));
  set_preds_succs(mockMBB3, arr({&mockMBB2}),arr({&mockMBB2,&mockMBB4}));
  set_preds_succs(mockMBB4, arr({&mockMBB3}),arr({}));

  MockLoop mockLoop1(nullptr,
      { &mockMBB2, &mockMBB3 },
      { &mockMBB3 },
      { &mockMBB3 }
  );
  LI.add_loop(&mockLoop1);

  auto result = memoryAccessCompensation(&mockMBB1, &LI, countAccess);

  EXPECT_THAT(
      result,
      UnorderedElementsAreArray({
        pair(pair(&mockMBB3,&mockMBB2), 87),
        pair(pair(&mockMBB3,&mockMBB4), 127),
      })
  );
}

TEST(MemoryAccessCompensationTest, NestedLoop){
  MockLoopInfo LI;

  mockMBB(mockMBB1,10,-1,-1);
  mockMBB(mockMBB2,20,1,7);
  mockMBB(mockMBB3,30,1,3);
  mockMBB(mockMBB4,40,-1,-1);
  mockMBB(mockMBB5,50,-1,-1);
  set_preds_succs(mockMBB1, arr({}), arr({&mockMBB2}));
  set_preds_succs(mockMBB2, arr({&mockMBB1,&mockMBB3}),arr({&mockMBB3,&mockMBB5}));
  set_preds_succs(mockMBB3, arr({&mockMBB2,&mockMBB4}),arr({&mockMBB2,&mockMBB4}));
  set_preds_succs(mockMBB4, arr({&mockMBB3}),arr({&mockMBB3}));
  set_preds_succs(mockMBB5, arr({&mockMBB2}), arr({}));

  MockLoop mockLoop1(nullptr,
      { &mockMBB2, &mockMBB3, &mockMBB4 },
      { &mockMBB3 },
      { &mockMBB2 }
  );
  MockLoop mockLoop2(&mockLoop1,
      { &mockMBB3, &mockMBB4 },
      { &mockMBB4 },
      { &mockMBB3 }
  );
  LI.add_loop(&mockLoop1);
  LI.add_loop(&mockLoop2);

  auto result = memoryAccessCompensation(&mockMBB1, &LI, countAccess);

  EXPECT_THAT(
      result,
      UnorderedElementsAreArray({
        pair(pair(&mockMBB2,&mockMBB5), 80),
        pair(pair(&mockMBB3,&mockMBB2), 50),
        pair(pair(&mockMBB4,&mockMBB3), 70),
      })
  );
}

TEST(MemoryAccessCompensationTest, TwoNonLatchHeaderPreds){
  MockLoopInfo LI;

  mockMBB(mockMBB1,1,-1,-1);
  mockMBB(mockMBB2,0,-1,-1);
  mockMBB(mockMBB3,10,-1,-1);
  mockMBB(mockMBB4,7,1,5);
  mockMBB(mockMBB5,3,-1,-1);

  set_preds_succs(mockMBB1, arr({}),arr({&mockMBB2,&mockMBB3}));
  set_preds_succs(mockMBB2, arr({&mockMBB1}),arr({&mockMBB4}));
  set_preds_succs(mockMBB3, arr({&mockMBB1}),arr({&mockMBB4}));
  set_preds_succs(mockMBB4, arr({&mockMBB2,&mockMBB3}),arr({&mockMBB4,&mockMBB5}));
  set_preds_succs(mockMBB5, arr({&mockMBB4}),arr({}));

  MockLoop mockLoop1(nullptr,
      { &mockMBB4 },
      { &mockMBB4 },
      { &mockMBB4 }
  );
  LI.add_loop(&mockLoop1);

  auto result = memoryAccessCompensation(&mockMBB1, &LI, countAccess);

  EXPECT_THAT(
      result,
      UnorderedElementsAreArray({
        pair(pair(&mockMBB2,&mockMBB4), 1),
        pair(pair(&mockMBB3,&mockMBB4), 11),
        pair(pair(&mockMBB4,&mockMBB4), 7),
        pair(pair(&mockMBB4,&mockMBB5), 10),
      })
  );
}

TEST(MemoryAccessCompensationTest, LoopExitFromNested){
  MockLoopInfo LI;

  mockMBB(mockMBB1,12,-1,-1);
  mockMBB(mockMBB2,23,1,16);
  mockMBB(mockMBB3,34,1,6);
  mockMBB(mockMBB4,45,-1,-1);
  mockMBB(mockMBB5,56,-1,-1);
  set_preds_succs(mockMBB1, arr({}), arr({&mockMBB2}));
  set_preds_succs(mockMBB2, arr({&mockMBB1,&mockMBB3}),arr({&mockMBB3,&mockMBB5}));
  set_preds_succs(mockMBB3, arr({&mockMBB2,&mockMBB4}),arr({&mockMBB2,&mockMBB4}));
  set_preds_succs(mockMBB4, arr({&mockMBB3}),arr({&mockMBB3,&mockMBB5}));
  set_preds_succs(mockMBB5, arr({&mockMBB2,&mockMBB4}), arr({}));

  MockLoop mockLoop1(nullptr,
      { &mockMBB2, &mockMBB3, &mockMBB4 },
      { &mockMBB3 },
      { &mockMBB2, &mockMBB4 }
  );
  MockLoop mockLoop2(&mockLoop1,
      { &mockMBB3, &mockMBB4 },
      { &mockMBB4 },
      { &mockMBB3, &mockMBB4 }
  );
  LI.add_loop(&mockLoop1);
  LI.add_loop(&mockLoop2);

  auto result = memoryAccessCompensation(&mockMBB1, &LI, countAccess);

  EXPECT_THAT(
      result,
      UnorderedElementsAreArray({
        pair(pair(&mockMBB2,&mockMBB5), 91),
        pair(pair(&mockMBB3,&mockMBB2), 57),
        pair(pair(&mockMBB4,&mockMBB3), 79),
        pair(pair(&mockMBB4,&mockMBB5), 170),
      })
  );
}

TEST(MemoryAccessCompensationTest, LoopLatchFromNested){
  MockLoopInfo LI;

  mockMBB(mockMBB1,11,-1,-1);
  mockMBB(mockMBB2,22,1,2);
  mockMBB(mockMBB3,33,1,4);
  mockMBB(mockMBB4,44,-1,-1);
  mockMBB(mockMBB5,55,-1,-1);
  set_preds_succs(mockMBB1, arr({}), arr({&mockMBB2}));
  set_preds_succs(mockMBB2, arr({&mockMBB1,&mockMBB3,&mockMBB4}),arr({&mockMBB3,&mockMBB5}));
  set_preds_succs(mockMBB3, arr({&mockMBB2,&mockMBB4}),arr({&mockMBB2,&mockMBB4}));
  set_preds_succs(mockMBB4, arr({&mockMBB3}),arr({&mockMBB3,&mockMBB2}));
  set_preds_succs(mockMBB5, arr({&mockMBB2}), arr({}));

  MockLoop mockLoop1(nullptr,
      {&mockMBB2,&mockMBB3,&mockMBB4},
      {&mockMBB3,&mockMBB4},
      {&mockMBB2}
  );
  MockLoop mockLoop2(&mockLoop1,
      {&mockMBB3,&mockMBB4},
      {&mockMBB4},
      {&mockMBB3,&mockMBB4}
  );
  LI.add_loop(&mockLoop1);
  LI.add_loop(&mockLoop2);

  auto result = memoryAccessCompensation(&mockMBB1, &LI, countAccess);

  EXPECT_THAT(
      result,
      UnorderedElementsAreArray({
        pair(pair(&mockMBB2,&mockMBB5), 88),
        pair(pair(&mockMBB3,&mockMBB2), 55),
        pair(pair(&mockMBB4,&mockMBB3), 77),
        pair(pair(&mockMBB4,&mockMBB2), 99),
      })
  );
}

TEST(MemoryAccessCompensationTest, TwoConsecutiveLoops){
  MockLoopInfo LI;

  mockMBB(mockMBB1,3,-1,-1);
  mockMBB(mockMBB2,1,1,10);
  mockMBB(mockMBB3,6,1,2);
  mockMBB(mockMBB4,10,-1,-1);
  set_preds_succs(mockMBB1, arr({}), arr({&mockMBB2}));
  set_preds_succs(mockMBB2, arr({&mockMBB1,&mockMBB2}),arr({&mockMBB2,&mockMBB3}));
  set_preds_succs(mockMBB3, arr({&mockMBB2,&mockMBB3}),arr({&mockMBB3,&mockMBB4}));
  set_preds_succs(mockMBB4, arr({&mockMBB3}),arr({}));

  MockLoop mockLoop1(nullptr,
      { &mockMBB2 },
      { &mockMBB2 },
      { &mockMBB2 }
  );
  MockLoop mockLoop2(nullptr,
      { &mockMBB3 },
      { &mockMBB3 },
      { &mockMBB3 }
  );
  LI.add_loop(&mockLoop1);
  LI.add_loop(&mockLoop2);

  auto result = memoryAccessCompensation(&mockMBB1, &LI, countAccess);

  EXPECT_THAT(
      result,
      UnorderedElementsAreArray({
        pair(pair(&mockMBB2,&mockMBB2), 1),
        pair(pair(&mockMBB3,&mockMBB3), 6),
        pair(pair(&mockMBB3,&mockMBB4), 20),
      })
  );
}

TEST(MemoryAccessCompensationTest, LoopExitToNonEnd){
  MockLoopInfo LI;

  mockMBB(mockMBB1,2,-1,-1);
  mockMBB(mockMBB2,1,1,10);
  mockMBB(mockMBB3,3,1,2);
  mockMBB(mockMBB4,10,-1,-1);
  set_preds_succs(mockMBB1, arr({}), arr({&mockMBB2}));
  set_preds_succs(mockMBB2, arr({&mockMBB1,&mockMBB2}),arr({&mockMBB2,&mockMBB3}));
  set_preds_succs(mockMBB3, arr({&mockMBB2}),arr({&mockMBB4}));
  set_preds_succs(mockMBB4, arr({&mockMBB3}),arr({}));

  MockLoop mockLoop1(nullptr,
      { &mockMBB2 },
      { &mockMBB2 },
      { &mockMBB2 }
  );
  LI.add_loop(&mockLoop1);

  auto result = memoryAccessCompensation(&mockMBB1, &LI, countAccess);

  EXPECT_THAT(
      result,
      UnorderedElementsAreArray({
        pair(pair(&mockMBB2,&mockMBB2), 1),
        pair(pair(&mockMBB3,&mockMBB4), 16),
      })
  );
}
}
