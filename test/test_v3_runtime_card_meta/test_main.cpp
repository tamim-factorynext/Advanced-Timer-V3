#include <unity.h>

#include "../../src/runtime/runtime_card_meta.cpp"

void setUp() {}
void tearDown() {}

void test_make_runtime_card_meta_copies_identity_and_mode() {
  LogicCard card = {};
  card.id = 9;
  card.type = SoftIO;
  card.index = 1;
  card.mode = Mode_DO_Immediate;

  RuntimeCardMeta meta = makeRuntimeCardMeta(card);
  TEST_ASSERT_EQUAL_UINT8(9, meta.id);
  TEST_ASSERT_EQUAL(SoftIO, meta.type);
  TEST_ASSERT_EQUAL_UINT8(1, meta.index);
  TEST_ASSERT_EQUAL(Mode_DO_Immediate, meta.mode);
}

void test_refresh_runtime_card_meta_from_cards() {
  LogicCard cards[2] = {};
  cards[0].id = 0;
  cards[0].type = DigitalInput;
  cards[0].index = 0;
  cards[0].mode = Mode_DI_Rising;
  cards[1].id = 10;
  cards[1].type = MathCard;
  cards[1].index = 0;
  cards[1].mode = Mode_None;

  RuntimeCardMeta out[2] = {};
  refreshRuntimeCardMetaFromCards(cards, 2, out);

  TEST_ASSERT_EQUAL_UINT8(0, out[0].id);
  TEST_ASSERT_EQUAL(DigitalInput, out[0].type);
  TEST_ASSERT_EQUAL(Mode_DI_Rising, out[0].mode);
  TEST_ASSERT_EQUAL_UINT8(10, out[1].id);
  TEST_ASSERT_EQUAL(MathCard, out[1].type);
  TEST_ASSERT_EQUAL(Mode_None, out[1].mode);
}

void test_refresh_runtime_card_meta_from_typed_cards() {
  V3CardConfig cards[3] = {};
  cards[0].cardId = 0;
  cards[0].family = V3CardFamily::DI;
  cards[0].di.channel = 0;
  cards[0].di.edgeMode = Mode_DI_Falling;

  cards[1].cardId = 4;
  cards[1].family = V3CardFamily::DO;
  cards[1].dout.channel = 0;
  cards[1].dout.mode = Mode_DO_Gated;

  cards[2].cardId = 16;
  cards[2].family = V3CardFamily::RTC;

  RuntimeCardMeta out[3] = {};
  refreshRuntimeCardMetaFromTypedCards(cards, 3, 4, 8, 10, 14, 16, out);

  TEST_ASSERT_EQUAL_UINT8(0, out[0].id);
  TEST_ASSERT_EQUAL(DigitalInput, out[0].type);
  TEST_ASSERT_EQUAL_UINT8(0, out[0].index);
  TEST_ASSERT_EQUAL(Mode_DI_Falling, out[0].mode);

  TEST_ASSERT_EQUAL_UINT8(4, out[1].id);
  TEST_ASSERT_EQUAL(DigitalOutput, out[1].type);
  TEST_ASSERT_EQUAL_UINT8(0, out[1].index);
  TEST_ASSERT_EQUAL(Mode_DO_Gated, out[1].mode);

  TEST_ASSERT_EQUAL_UINT8(16, out[2].id);
  TEST_ASSERT_EQUAL(RtcCard, out[2].type);
  TEST_ASSERT_EQUAL_UINT8(0, out[2].index);
  TEST_ASSERT_EQUAL(Mode_None, out[2].mode);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_make_runtime_card_meta_copies_identity_and_mode);
  RUN_TEST(test_refresh_runtime_card_meta_from_cards);
  RUN_TEST(test_refresh_runtime_card_meta_from_typed_cards);
  return UNITY_END();
}
