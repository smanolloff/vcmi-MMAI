#include "BAI/v12/encoder.h"
#include "schema/v12/constants.h"
#include "schema/v12/types.h"
#include "test/googletest/googletest/include/gtest/gtest.h"
#include <stdexcept>

using Encoder = MMAI::BAI::V12::Encoder;
using namespace MMAI::Schema::V12;


TEST(Encoder, Encode) {
  {
    constexpr auto a = HexAttribute::Y_COORD;
    constexpr auto e = std::get<1>(HEX_ENCODING.at(int(a)));
    constexpr auto n = std::get<2>(HEX_ENCODING.at(int(a)));
    static_assert(e == Encoding::CATEGORICAL_STRICT_NULL, "test needs to be updated");
    static_assert(n == 11, "test needs to be updated");

    {
        auto have = std::vector<float> {};
        auto want = std::vector<float> {1,0,0,0,0,0,0,0,0,0,0};
        Encoder::Encode(a, 0, have);
        ASSERT_EQ(want, have);
    }
    {
        auto have = std::vector<float> {};
        auto want = std::vector<float> {0,0,0,0,0,0,0,0,0,1,0};
        Encoder::Encode(a, 9, have);
        ASSERT_EQ(want, have);
    }
    {
        auto have = std::vector<float> {};
        ASSERT_THROW(Encoder::Encode(a, -1, have), std::runtime_error);
    }

    // This no longer throws (emits warning instead)
    // {
    //     auto have = std::vector<float> {};
    //     ASSERT_THROW(Encoder::Encode(a, 666, have), std::runtime_error);
    // }
  }
}

TEST(Encoder, AccumulatingExplicitNull) {
  static_assert(EI(Encoding::ACCUMULATING_EXPLICIT_NULL) == 0, "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,1,1,1,1};
    Encoder::EncodeAccumulatingExplicitNull(3, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,1,0,0,0};
    Encoder::EncodeAccumulatingExplicitNull(0, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,0,0,0,0};
    Encoder::EncodeAccumulatingExplicitNull(-1, 5, have);
    ASSERT_EQ(want, have);
  }
}

TEST(Encoder, AccumulatingImplicitNull) {
  static_assert(EI(Encoding::ACCUMULATING_IMPLICIT_NULL) == 1+EI(Encoding::ACCUMULATING_EXPLICIT_NULL), "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,1,1,1,0};
    Encoder::EncodeAccumulatingImplicitNull(3, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,0,0,0,0};
    Encoder::EncodeAccumulatingImplicitNull(0, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,0,0,0,0};
    Encoder::EncodeAccumulatingImplicitNull(-1, 5, have);
    ASSERT_EQ(want, have);
  }
}

TEST(Encoder, AccumulatingMaskingNull) {
  static_assert(EI(Encoding::ACCUMULATING_MASKING_NULL) == 1+EI(Encoding::ACCUMULATING_IMPLICIT_NULL), "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,1,1,1,0};
    Encoder::EncodeAccumulatingMaskingNull(3, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,0,0,0,0};
    Encoder::EncodeAccumulatingMaskingNull(0, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {-1,-1,-1,-1,-1};
    Encoder::EncodeAccumulatingMaskingNull(-1, 5, have);
    ASSERT_EQ(want, have);
  }
}

TEST(Encoder, AccumulatingStrictNull) {
  static_assert(EI(Encoding::ACCUMULATING_STRICT_NULL) == 1+EI(Encoding::ACCUMULATING_MASKING_NULL), "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,1,1,1,0};
    Encoder::EncodeAccumulatingStrictNull(3, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,0,0,0,0};
    Encoder::EncodeAccumulatingStrictNull(0, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    ASSERT_THROW(Encoder::EncodeAccumulatingStrictNull(-1, 5, have), std::runtime_error);
  }
}

TEST(Encoder, AccumulatingZeroNull) {
  static_assert(EI(Encoding::ACCUMULATING_ZERO_NULL) == 1+EI(Encoding::ACCUMULATING_STRICT_NULL), "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,1,1,1,0};
    Encoder::EncodeAccumulatingZeroNull(3, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,0,0,0,0};
    Encoder::EncodeAccumulatingZeroNull(0, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,0,0,0,0};
    Encoder::EncodeAccumulatingZeroNull(-1, 5, have);
    ASSERT_EQ(want, have);
  }
}

TEST(Encoder, BinaryExplicitNull) {
  static_assert(EI(Encoding::BINARY_EXPLICIT_NULL) == 1+EI(Encoding::ACCUMULATING_ZERO_NULL), "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,1,1,0,0};
    Encoder::EncodeBinaryExplicitNull(0b11, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,0,0,0,0};
    Encoder::EncodeBinaryExplicitNull(0, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,0,0,0,0};
    Encoder::EncodeBinaryExplicitNull(-1, 5, have);
    ASSERT_EQ(want, have);
  }
}

TEST(Encoder, BinaryMaskingNull) {
  static_assert(EI(Encoding::BINARY_MASKING_NULL) == 1+EI(Encoding::BINARY_EXPLICIT_NULL), "Encoding list has changed");

  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,1,0,0,0};
    Encoder::EncodeBinaryMaskingNull(0b11, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,0,0,0,0};
    Encoder::EncodeBinaryMaskingNull(0, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {-1,-1,-1,-1,-1};
    Encoder::EncodeBinaryMaskingNull(-1, 5, have);
    ASSERT_EQ(want, have);
  }
}

TEST(Encoder, BinaryStrictNull) {
  static_assert(EI(Encoding::BINARY_STRICT_NULL) == 1+EI(Encoding::BINARY_MASKING_NULL), "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,1,0,0,0};
    Encoder::EncodeBinaryStrictNull(0b11, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,0,0,0,0};
    Encoder::EncodeBinaryStrictNull(0, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    ASSERT_THROW(Encoder::EncodeBinaryStrictNull(-1, 5, have), std::runtime_error);
  }
}

TEST(Encoder, BinaryZeroNull) {
  static_assert(EI(Encoding::BINARY_ZERO_NULL) == 1+EI(Encoding::BINARY_STRICT_NULL), "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,1,0,0,0};
    Encoder::EncodeBinaryZeroNull(0b11, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,0,0,0,0};
    Encoder::EncodeBinaryZeroNull(0, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,0,0,0,0};
    Encoder::EncodeBinaryZeroNull(-1, 5, have);
    ASSERT_EQ(want, have);
  }
}

TEST(Encoder, CategoricalExplicitNull) {
  static_assert(EI(Encoding::CATEGORICAL_EXPLICIT_NULL) == 1+EI(Encoding::BINARY_ZERO_NULL), "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,0,0,0,1};
    Encoder::EncodeCategoricalExplicitNull(3, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,1,0,0,0};
    Encoder::EncodeCategoricalExplicitNull(0, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,0,0,0,0};
    Encoder::EncodeCategoricalExplicitNull(-1, 5, have);
    ASSERT_EQ(want, have);
  }
}

TEST(Encoder, CategoricalImplicitNull) {
  static_assert(EI(Encoding::CATEGORICAL_IMPLICIT_NULL) == 1+EI(Encoding::CATEGORICAL_EXPLICIT_NULL), "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,0,0,1,0};
    Encoder::EncodeCategoricalImplicitNull(3, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,0,0,0,0};
    Encoder::EncodeCategoricalImplicitNull(0, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,0,0,0,0};
    Encoder::EncodeCategoricalImplicitNull(-1, 5, have);
    ASSERT_EQ(want, have);
  }
}

TEST(Encoder, CategoricalMaskingNull) {
  static_assert(EI(Encoding::CATEGORICAL_MASKING_NULL) == 1+EI(Encoding::CATEGORICAL_IMPLICIT_NULL), "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,0,0,1,0};
    Encoder::EncodeCategoricalMaskingNull(3, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,0,0,0,0};
    Encoder::EncodeCategoricalMaskingNull(0, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {-1,-1,-1,-1,-1};
    Encoder::EncodeCategoricalMaskingNull(-1, 5, have);
    ASSERT_EQ(want, have);
  }
}

TEST(Encoder, CategoricalStrictNull) {
  static_assert(EI(Encoding::CATEGORICAL_STRICT_NULL) == 1+EI(Encoding::CATEGORICAL_MASKING_NULL), "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,0,0,1,0};
    Encoder::EncodeCategoricalStrictNull(3, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,0,0,0,0};
    Encoder::EncodeCategoricalStrictNull(0, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    ASSERT_THROW(Encoder::EncodeCategoricalStrictNull(-1, 5, have), std::runtime_error);
  }
}

TEST(Encoder, CategoricalZeroNull) {
  static_assert(EI(Encoding::CATEGORICAL_ZERO_NULL) == 1+EI(Encoding::CATEGORICAL_STRICT_NULL), "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,0,0,1,0};
    Encoder::EncodeCategoricalZeroNull(3, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,0,0,0,0};
    Encoder::EncodeCategoricalZeroNull(0, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,0,0,0,0};
    Encoder::EncodeCategoricalZeroNull(-1, 5, have);
    ASSERT_EQ(want, have);
  }
}

TEST(Encoder, NormalizedExpExplicitNull) {
  static_assert(EI(Encoding::EXPNORM_EXPLICIT_NULL) == 1+EI(Encoding::CATEGORICAL_ZERO_NULL), "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0, 0.876};
    Encoder::EncodeExpnormExplicitNull(3, 5, 4, have);
    ASSERT_EQ(have.size(), 2);
    ASSERT_EQ(want.at(0), have.at(0));
    ASSERT_NEAR(want.at(1), have.at(1), 1e-3);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0, 0};
    Encoder::EncodeExpnormExplicitNull(0, 5, 4, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1, 0};
    Encoder::EncodeExpnormExplicitNull(-1, 5, 4, have);
    ASSERT_EQ(want, have);
  }
}

TEST(Encoder, NormalizedExpMaskingNull) {
  static_assert(EI(Encoding::EXPNORM_MASKING_NULL) == 1+EI(Encoding::EXPNORM_EXPLICIT_NULL), "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0.876};
    Encoder::EncodeExpnormMaskingNull(3, 5, 4, have);
    ASSERT_EQ(have.size(), 1);
    ASSERT_NEAR(want.at(0), have.at(0), 1e-3);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0};
    Encoder::EncodeExpnormMaskingNull(0, 5, 4, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {-1};
    Encoder::EncodeExpnormMaskingNull(-1, 5, 4, have);
    ASSERT_EQ(want, have);
  }
}

TEST(Encoder, NormalizedExpStrictNull) {
  static_assert(EI(Encoding::EXPNORM_STRICT_NULL) == 1+EI(Encoding::EXPNORM_MASKING_NULL), "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0.876};
    Encoder::EncodeExpnormStrictNull(3, 5, 4, have);
    ASSERT_EQ(have.size(), 1);
    ASSERT_NEAR(want.at(0), have.at(0), 1e-3);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0};
    Encoder::EncodeExpnormStrictNull(0, 5, 4, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    ASSERT_THROW(Encoder::EncodeExpnormStrictNull(-1, 5, 4, have), std::runtime_error);
  }
}

TEST(Encoder, NormalizedExpZeroNull) {
  static_assert(EI(Encoding::EXPNORM_ZERO_NULL) == 1+EI(Encoding::EXPNORM_STRICT_NULL), "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0.876};
    Encoder::EncodeExpnormZeroNull(3, 5, 4, have);
    ASSERT_EQ(have.size(), 1);
    ASSERT_NEAR(want.at(0), have.at(0), 1e-3);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0};
    Encoder::EncodeExpnormZeroNull(0, 5, 4, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0};
    Encoder::EncodeExpnormZeroNull(-1, 5, 4, have);
    ASSERT_EQ(want, have);
  }
}

TEST(Encoder, NormalizedLinExplicitNull) {
  static_assert(EI(Encoding::LINNORM_EXPLICIT_NULL) == 1+EI(Encoding::EXPNORM_ZERO_NULL), "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0, 0.6};
    Encoder::EncodeLinnormExplicitNull(3, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0, 0};
    Encoder::EncodeLinnormExplicitNull(0, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1, 0};
    Encoder::EncodeLinnormExplicitNull(-1, 5, have);
    ASSERT_EQ(want, have);
  }
}

TEST(Encoder, NormalizedMaskingNull) {
  static_assert(EI(Encoding::LINNORM_MASKING_NULL) == 1+EI(Encoding::LINNORM_EXPLICIT_NULL), "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0.6};
    Encoder::EncodeLinnormMaskingNull(3, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0};
    Encoder::EncodeLinnormMaskingNull(0, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {-1};
    Encoder::EncodeLinnormMaskingNull(-1, 5, have);
    ASSERT_EQ(want, have);
  }
}

TEST(Encoder, NormalizedStrictNull) {
  static_assert(EI(Encoding::LINNORM_STRICT_NULL) == 1+EI(Encoding::LINNORM_MASKING_NULL), "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0.6};
    Encoder::EncodeLinnormStrictNull(3, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0};
    Encoder::EncodeLinnormStrictNull(0, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    ASSERT_THROW(Encoder::EncodeLinnormStrictNull(-1, 5, have), std::runtime_error);
  }
}

TEST(Encoder, NormalizedZeroNull) {
  static_assert(EI(Encoding::LINNORM_ZERO_NULL) == 1+EI(Encoding::LINNORM_STRICT_NULL), "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0.6};
    Encoder::EncodeLinnormZeroNull(3, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0};
    Encoder::EncodeLinnormZeroNull(0, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0};
    Encoder::EncodeLinnormZeroNull(-1, 5, have);
    ASSERT_EQ(want, have);
  }
}

TEST(Encoder, ExpbinExplicitNull) {
  static_assert(EI(Encoding::EXPBIN_EXPLICIT_NULL) == 1+EI(Encoding::LINNORM_ZERO_NULL), "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,0,0,0,0,0,0};
    Encoder::EncodeExpbinExplicitNull(-1, 1+6, 80, 6.5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,1,0,0,0,0,0};
    Encoder::EncodeExpbinExplicitNull(0, 1+6, 80, 6.5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,0,0,0,1,0,0};
    Encoder::EncodeExpbinExplicitNull(3, 1+6, 80, 6.5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,0,0,0,0,0,1};
    Encoder::EncodeExpbinExplicitNull(666, 1+6, 80, 6.5, have);
    ASSERT_EQ(want, have);
  }
}

TEST(Encoder, ExpbinImplicitNull) {
  static_assert(EI(Encoding::EXPBIN_IMPLICIT_NULL) == 1+EI(Encoding::EXPBIN_EXPLICIT_NULL), "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,0,0,0,0,0};
    Encoder::EncodeExpbinImplicitNull(-1, 6, 80, 6.5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,0,0,0,0,0};
    Encoder::EncodeExpbinImplicitNull(0, 6, 80, 6.5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,0,0,1,0,0};
    Encoder::EncodeExpbinImplicitNull(3, 6, 80, 6.5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,0,0,1,0,0};
    Encoder::EncodeExpbinImplicitNull(8, 6, 80, 6.5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,0,0,0,1,0};
    Encoder::EncodeExpbinImplicitNull(10, 6, 80, 6.5, have);
    ASSERT_EQ(want, have);
  }
}


TEST(Encoder, ExpbinMaskingNull) {
  static_assert(EI(Encoding::EXPBIN_MASKING_NULL) == 1+EI(Encoding::EXPBIN_IMPLICIT_NULL), "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {-1,-1,-1,-1,-1,-1};
    Encoder::EncodeExpbinMaskingNull(-1, 6, 80, 6.5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,0,0,0,0,0};
    Encoder::EncodeExpbinMaskingNull(0, 6, 80, 6.5, have);
    ASSERT_EQ(want, have);
  }
}

TEST(Encoder, ExpbinStrictNull) {
  static_assert(EI(Encoding::EXPBIN_STRICT_NULL) == 1+EI(Encoding::EXPBIN_MASKING_NULL), "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    ASSERT_THROW(Encoder::EncodeExpbinStrictNull(-1, 6, 80, 6.5, have), std::runtime_error);
  }

  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,0,0,0,0,0};
    Encoder::EncodeExpbinStrictNull(0, 6, 80, 6.5, have);
    ASSERT_EQ(want, have);
  }
}

TEST(Encoder, ExpbinZeroNull) {
  static_assert(EI(Encoding::EXPBIN_ZERO_NULL) == 1+EI(Encoding::EXPBIN_STRICT_NULL), "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,0,0,0,0,0};
    Encoder::EncodeExpbinZeroNull(-1, 6, 80, 6.5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,0,0,0,0,0};
    Encoder::EncodeExpbinZeroNull(0, 6, 80, 6.5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,0,0,1,0,0};
    Encoder::EncodeExpbinZeroNull(3, 6, 80, 6.5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,0,0,0,0,1};
    Encoder::EncodeExpbinZeroNull(666, 6, 80, 6.5, have);
    ASSERT_EQ(want, have);
  }
}


TEST(Encoder, AccumulatingExpbinExplicitNull) {
  static_assert(EI(Encoding::ACCUMULATING_EXPBIN_EXPLICIT_NULL) == 1+EI(Encoding::EXPBIN_ZERO_NULL), "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,0,0,0,0,0,0};
    Encoder::EncodeAccumulatingExpbinExplicitNull(-1, 1+6, 80, 6.5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,1,0,0,0,0,0};
    Encoder::EncodeAccumulatingExpbinExplicitNull(0, 1+6, 80, 6.5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,1,1,1,1,0,0};
    Encoder::EncodeAccumulatingExpbinExplicitNull(3, 1+6, 80, 6.5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,1,1,1,1,1,1};
    Encoder::EncodeAccumulatingExpbinExplicitNull(666, 1+6, 80, 6.5, have);
    ASSERT_EQ(want, have);
  }
}

TEST(Encoder, AccumulatingExpbinImplicitNull) {
  static_assert(EI(Encoding::ACCUMULATING_EXPBIN_IMPLICIT_NULL) == 1+EI(Encoding::ACCUMULATING_EXPBIN_EXPLICIT_NULL), "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,0,0,0,0,0};
    Encoder::EncodeAccumulatingExpbinImplicitNull(-1, 6, 80, 6.5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,0,0,0,0,0};
    Encoder::EncodeAccumulatingExpbinImplicitNull(0, 6, 80, 6.5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,1,1,1,0,0};
    Encoder::EncodeAccumulatingExpbinImplicitNull(3, 6, 80, 6.5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,1,1,1,0,0};
    Encoder::EncodeAccumulatingExpbinImplicitNull(8, 6, 80, 6.5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,1,1,1,1,0};
    Encoder::EncodeAccumulatingExpbinImplicitNull(10, 6, 80, 6.5, have);
    ASSERT_EQ(want, have);
  }
}


TEST(Encoder, AccumulatingExpbinMaskingNull) {
  static_assert(EI(Encoding::ACCUMULATING_EXPBIN_MASKING_NULL) == 1+EI(Encoding::ACCUMULATING_EXPBIN_IMPLICIT_NULL), "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {-1,-1,-1,-1,-1,-1};
    Encoder::EncodeAccumulatingExpbinMaskingNull(-1, 6, 80, 6.5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,0,0,0,0,0};
    Encoder::EncodeAccumulatingExpbinMaskingNull(0, 6, 80, 6.5, have);
    ASSERT_EQ(want, have);
  }
}

TEST(Encoder, AccumulatingExpbinStrictNull) {
  static_assert(EI(Encoding::ACCUMULATING_EXPBIN_STRICT_NULL) == 1+EI(Encoding::ACCUMULATING_EXPBIN_MASKING_NULL), "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    ASSERT_THROW(Encoder::EncodeAccumulatingExpbinStrictNull(-1, 6, 80, 6.5, have), std::runtime_error);
  }

  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,0,0,0,0,0};
    Encoder::EncodeAccumulatingExpbinStrictNull(0, 6, 80, 6.5, have);
    ASSERT_EQ(want, have);
  }
}

TEST(Encoder, AccumulatingExpbinZeroNull) {
  static_assert(EI(Encoding::ACCUMULATING_EXPBIN_ZERO_NULL) == 1+EI(Encoding::ACCUMULATING_EXPBIN_STRICT_NULL), "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,0,0,0,0,0};
    Encoder::EncodeAccumulatingExpbinZeroNull(-1, 6, 80, 6.5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,0,0,0,0,0};
    Encoder::EncodeAccumulatingExpbinZeroNull(0, 6, 80, 6.5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,1,1,1,0,0};
    Encoder::EncodeAccumulatingExpbinZeroNull(3, 6, 80, 6.5, have);
    ASSERT_EQ(want, have);
  }
}

TEST(Encoder, LinbinExplicitNull) {
  static_assert(EI(Encoding::LINBIN_EXPLICIT_NULL) == 1+EI(Encoding::ACCUMULATING_EXPBIN_ZERO_NULL), "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,0,0,0};
    Encoder::EncodeLinbinExplicitNull(-1, 1+3, 15, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,1,0,0};
    Encoder::EncodeLinbinExplicitNull(0, 1+3, 15, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,1,0,0};
    Encoder::EncodeLinbinExplicitNull(3, 1+3, 15, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,0,1,0};
    Encoder::EncodeLinbinExplicitNull(9, 1+3, 15, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,0,0,1};
    Encoder::EncodeLinbinExplicitNull(10, 1+3, 15, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,0,0,1};
    Encoder::EncodeLinbinExplicitNull(666, 1+3, 15, 5, have);
    ASSERT_EQ(want, have);
  }
}

TEST(Encoder, LinbinImplicitNull) {
  static_assert(EI(Encoding::LINBIN_IMPLICIT_NULL) == 1+EI(Encoding::LINBIN_EXPLICIT_NULL), "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,0,0};
    Encoder::EncodeLinbinImplicitNull(-1, 3, 15, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,0,0};
    Encoder::EncodeLinbinImplicitNull(0, 3, 15, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,0,0};
    Encoder::EncodeLinbinImplicitNull(3, 3, 15, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,1,0};
    Encoder::EncodeLinbinImplicitNull(5, 3, 15, 5, have);
    ASSERT_EQ(want, have);
  }
}


TEST(Encoder, LinbinMaskingNull) {
  static_assert(EI(Encoding::LINBIN_MASKING_NULL) == 1+EI(Encoding::LINBIN_IMPLICIT_NULL), "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {-1,-1,-1};
    Encoder::EncodeLinbinMaskingNull(-1, 3, 15, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,0,0};
    Encoder::EncodeLinbinMaskingNull(0, 3, 15, 5, have);
    ASSERT_EQ(want, have);
  }
}

TEST(Encoder, LinbinStrictNull) {
  static_assert(EI(Encoding::LINBIN_STRICT_NULL) == 1+EI(Encoding::LINBIN_MASKING_NULL), "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    ASSERT_THROW(Encoder::EncodeLinbinStrictNull(-1, 3, 15, 5, have), std::runtime_error);
  }

  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,0,0};
    Encoder::EncodeLinbinStrictNull(0, 3, 15, 5, have);
    ASSERT_EQ(want, have);
  }
}

TEST(Encoder, LinbinZeroNull) {
  static_assert(EI(Encoding::LINBIN_ZERO_NULL) == 1+EI(Encoding::LINBIN_STRICT_NULL), "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,0,0};
    Encoder::EncodeLinbinZeroNull(-1, 3, 15, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,0,0};
    Encoder::EncodeLinbinZeroNull(0, 3, 15, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,1,0};
    Encoder::EncodeLinbinZeroNull(5, 3, 15, 5, have);
    ASSERT_EQ(want, have);
  }
}


TEST(Encoder, AccumulatingLinbinExplicitNull) {
  static_assert(EI(Encoding::ACCUMULATING_LINBIN_EXPLICIT_NULL) == 1+EI(Encoding::LINBIN_ZERO_NULL), "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,0,0,0};
    Encoder::EncodeAccumulatingLinbinExplicitNull(-1, 1+3, 15, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,1,0,0};
    Encoder::EncodeAccumulatingLinbinExplicitNull(0, 1+3, 15, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,1,1,0};
    Encoder::EncodeAccumulatingLinbinExplicitNull(5, 1+3, 15, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,1,1,1};
    Encoder::EncodeAccumulatingLinbinExplicitNull(666, 1+3, 15, 5, have);
    ASSERT_EQ(want, have);
  }
}

TEST(Encoder, AccumulatingLinbinImplicitNull) {
  static_assert(EI(Encoding::ACCUMULATING_LINBIN_IMPLICIT_NULL) == 1+EI(Encoding::ACCUMULATING_LINBIN_EXPLICIT_NULL), "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {0,0,0};
    Encoder::EncodeAccumulatingLinbinImplicitNull(-1, 3, 15, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,0,0};
    Encoder::EncodeAccumulatingLinbinImplicitNull(0, 3, 15, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,1,0};
    Encoder::EncodeAccumulatingLinbinImplicitNull(5, 3, 15, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,1,1};
    Encoder::EncodeAccumulatingLinbinImplicitNull(666, 3, 15, 5, have);
    ASSERT_EQ(want, have);
  }
}


TEST(Encoder, AccumulatingLinbinMaskingNull) {
  static_assert(EI(Encoding::ACCUMULATING_LINBIN_MASKING_NULL) == 1+EI(Encoding::ACCUMULATING_LINBIN_IMPLICIT_NULL), "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {-1,-1,-1};
    Encoder::EncodeAccumulatingLinbinMaskingNull(-1, 3, 15, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,0,0};
    Encoder::EncodeAccumulatingLinbinMaskingNull(0, 3, 15, 5, have);
    ASSERT_EQ(want, have);
  }
}

TEST(Encoder, AccumulatingLinbinStrictNull) {
  static_assert(EI(Encoding::ACCUMULATING_LINBIN_STRICT_NULL) == 1+EI(Encoding::ACCUMULATING_LINBIN_MASKING_NULL), "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    ASSERT_THROW(Encoder::EncodeAccumulatingLinbinStrictNull(-1, 3, 15, 5, have), std::runtime_error);
  }

  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,0,0};
    Encoder::EncodeAccumulatingLinbinStrictNull(0, 3, 15, 5, have);
    ASSERT_EQ(want, have);
  }
}

TEST(Encoder, AccumulatingLinbinZeroNull) {
  static_assert(EI(Encoding::ACCUMULATING_LINBIN_ZERO_NULL) == 1+EI(Encoding::ACCUMULATING_LINBIN_STRICT_NULL), "Encoding list has changed");
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,0,0};
    Encoder::EncodeAccumulatingLinbinZeroNull(-1, 3, 15, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,0,0};
    Encoder::EncodeAccumulatingLinbinZeroNull(0, 3, 15, 5, have);
    ASSERT_EQ(want, have);
  }
  {
    auto have = std::vector<float> {};
    auto want = std::vector<float> {1,1,0};
    Encoder::EncodeAccumulatingLinbinZeroNull(5, 3, 15, 5, have);
    ASSERT_EQ(want, have);
  }
}
