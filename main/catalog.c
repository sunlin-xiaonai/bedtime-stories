#include "catalog.h"

extern const uint8_t khalai_start[] asm("_binary_khalai_bin_start");
extern const uint8_t khalai_end[] asm("_binary_khalai_bin_end");
extern const uint8_t read_start[] asm("_binary_read_bin_start");
extern const uint8_t read_end[] asm("_binary_read_bin_end");
extern const uint8_t counting_start[] asm("_binary_counting_bin_start");
extern const uint8_t counting_end[] asm("_binary_counting_bin_end");

const story_entry_t story_catalog[] = {
    {"0089", "和植物聊天的卡莱", "4237a8d360e998c2bd396769fe12f6c5754a204c2ee47a86009dfc88d1eae202", 287518, 0, 1, khalai_start, khalai_end},
    {"0087", "我喜欢看书", "df127003acade9400c5cb7987d49b92ea1f19edfa5afcd4b0f2f8a2203f5af83", 80983, 1, 1, read_start, read_end},
    {"0327", "数数动物", "7e3ef31195f02a3d30c1f78a68baec4f994a3f50c15635d134de00bad5c48f0b", 116981, 2, 1, counting_start, counting_end},
    {"0001", "一个很高的人", "a0f7f6b23810e514a1600076c78fa83822049703136a5d8cd9c9306502f33296", 147064, 0, 1, NULL, NULL},
    {"0004", "山羊、野狗、和水牛", "46ef964d118c750c5729182e0f2852625a80d7d3a7e325d60132961399c84b63", 207201, 1, 1, NULL, NULL},
    {"0006", "阿南西和智慧", "3930d360ef1552781d67b18c3f2da93e72037beff58cad25b907392a39ee31b9", 360328, 2, 1, NULL, NULL},
    {"0008", "你在做什么？", "7afe92a55ae5a381108274d3f2a62c418a696fc975534db1280b1b2c72c049c1", 45785, 0, 1, NULL, NULL},
    {"0027", "决定", "83e1143ffdf920a5915522f674d0400a8a90829579a4451d457de22c5e33ffef", 315621, 1, 0, NULL, NULL},
    {"0052", "辛波薇亚", "f366f499ebc8b92dd8d3518a10bcfa4b25cbaa3a5dcb73ca62750c08dd4809d6", 1296786, 2, 0, NULL, NULL},
    {"0066", "诺孜贝儿和三根头发", "44d6787497d3bea38a6805277ef21155d455ebecabd70597ffd56f96b68130aa", 569018, 0, 0, NULL, NULL},
    {"0072", "指蜜鸟的复仇", "321802e5dae8fffa17550cf0dd10f5eefb68088b71b49c8021efdefb7c413578", 986577, 1, 0, NULL, NULL},
    {"0095", "赞玛很棒！", "a9ebb845f34ae08b31a1dfcc9502d587441a680549b2c441bd35c8c529378313", 195039, 2, 1, NULL, NULL},
    {"0110", "一粒小种子：旺加里·马塔伊的故事", "b2ef0248e66889895b7608ae8666be726d6f8bd04cbcdfe15290b11b31d99ac6", 617943, 0, 0, NULL, NULL},
    {"0111", "为什么河马没有毛发", "98b2bda3d1fb698e8c2ca033d62f5127dba2dd8360bf479134cbe10543f6238a", 317889, 1, 0, NULL, NULL},
    {"0141", "鸡和千足虫", "ede1b149c942e310950fd3d855bdc909f9a1f8b8cb69cc0657abc4ded54b0464", 451855, 2, 0, NULL, NULL},
    {"0156", "很饿的鳄鱼", "c26ea5a205905bcf208d0ef0e6cac0df3be060185ce5896342c2c61568fa9c50", 73899, 0, 0, NULL, NULL},
    {"0158", "母鸡和老鹰", "f6c0ef364cc02c70bcb2d77a8df21492deb78629fce30d684ab49a48df69288a", 625217, 1, 0, NULL, NULL},
    {"0201", "驴孩子", "9c85064dd91e22ae11c1936409bce2db28a7f08ebfaf5b434b1215bfb62aacca", 784058, 2, 0, NULL, NULL},
    {"0210", "丁吉和奶牛", "4795f91170ec90d20b40d10633ac08d4b6c11dd4ebc29b0f91793cdd5f6171fb", 159091, 0, 0, NULL, NULL},
    {"0243", "和奶奶一起过暑假", "1cc027ccc3e6e56b1534fb179149949bfc7a632eeddf75ff2196c1ee4df20ad5", 919522, 1, 1, NULL, NULL},
    {"0262", "玛格威", "9f3349353fc8f2a5c8764b124fc447a5fa88fd8198e13b64b9ea10329f285f53", 1241385, 2, 0, NULL, NULL},
    {"0291", "福思的姐姐说了什么", "5033af90478cab80b6a093cd94183ccb97ad14be9f71b406505064a72429cb87", 794981, 0, 1, NULL, NULL},
    {"0294", "奶奶的香蕉", "0931014a203731183df8f8fa6069209c8cd69bca45ef9c02177ace182a485953", 762113, 1, 1, NULL, NULL},
    {"0296", "卖香蕉的汤姆", "1e02c98bd9bea9d2a6a592d00535a31e4d9d9d04d28edf10b8a23f75b14cbe0e", 246753, 2, 1, NULL, NULL},
    {"0302", "火", "cdbe224dad6473c5956f3cef1c200126683016110f111118385b09039fe101c8", 61045, 0, 1, NULL, NULL},
    {"0315", "萨可满的歌声", "626989241b679205fbddc0cf3e8e57c9c25b3e582a64083ee7a99c2e6dab3ec3", 695934, 1, 0, NULL, NULL},
    {"0324", "离家进城的那一天", "05223c00d7bc83f0f179fe303a0b785d0a9eb8cadbcc4214abda80173b89d428", 619560, 2, 0, NULL, NULL},
    {"0337", "蜡做的孩子", "f390859cb359188ada3b504d67aa0b8319159b26b85235e2da30737c6e147c4c", 229680, 0, 0, NULL, NULL},
    {"0342", "惩罚", "3d320bfdebe074f37eb0e86b6900eaa60aa2531ca34f155418fc508f9bb21117", 229166, 1, 0, NULL, NULL},
    {"9001", "托马斯英文·托马斯与巴蒂", "509909f8060c064259956a747a99f1a3ef00240ae7f934d8c7eed3d256fdb342", 882049, 3, 1, NULL, NULL},
    {"9002", "托马斯英文·托马斯的列车", "3d63b73a98973f24032ddb4663326b2fdf3faff8a41fc1931e0d07fe3c82424c", 916101, 3, 1, NULL, NULL},
    {"9003", "托马斯英文·亨利和许愿树", "bfde57a207f71d305c41609cd83ae55de7533950b842c17951f1867635be6910", 1033552, 3, 1, NULL, NULL},
    {"9004", "托马斯英文·培西和风琴", "728e67e20930fe9e9c46a435f90118c64269827893e6e36d0cffaf61b413289a", 1449024, 3, 1, NULL, NULL},
    {"9005", "托马斯英文·培西和哈罗德", "f41e716bfc459e267b462f6bf04d6a30bc4f2bb6442289323ff85fe53852c452", 826345, 3, 1, NULL, NULL},
    {"9006", "托马斯英文·托马斯的圣诞派对", "1f2724236e95989e3a38048bacc1e1e3b748bf738d38fe24cbf6ebeef12c00bd", 1023389, 3, 1, NULL, NULL},
    {"9007", "托马斯英文·托马斯和布鲁诺", "78129d4bc90424d29b69db995171fadab2cd575de9e74f96532ac5a38ec32f1c", 913395, 3, 1, NULL, NULL},
    {"9008", "托马斯英文·高登脱轨了", "634afbc860371fdeb1b4ec238c2fef0130239d0174d45c24af1876663ceabf37", 974891, 3, 1, NULL, NULL},
    {"9101", "猫和老鼠英文·不是奶酪", "1fdfb94d438df0c9b20ea1ce42e7ac781b81db9148147a91dd142f42e7298cb6", 405249, 4, 1, NULL, NULL},
};
_Static_assert(sizeof(story_catalog) / sizeof(story_catalog[0]) == STORY_COUNT, "catalog count");
_Static_assert(CAT_INDEX + 1 == STORY_COUNT, "podcast catalog placement");
const size_t story_catalog_count = sizeof(story_catalog) / sizeof(story_catalog[0]);
