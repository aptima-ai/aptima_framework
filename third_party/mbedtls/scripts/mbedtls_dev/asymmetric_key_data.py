"""Sample key material for asymmetric key types.

Meant for use in crypto_knowledge.py.
"""

# Copyright The Mbed TLS Contributors
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import binascii
import re
from typing import Dict

STR_TRANS_REMOVE_BLANKS = str.maketrans("", "", " \t\n\r")


def unhexlify(text: str) -> bytes:
    return binascii.unhexlify(text.translate(STR_TRANS_REMOVE_BLANKS))


def construct_asymmetric_key_data(src) -> Dict[str, Dict[int, bytes]]:
    """Split key pairs into separate table entries and convert hex to bytes.

    Input format: src[abbreviated_type][size] = (private_key_hex, public_key_hex)
    Output format: dst['PSA_KEY_TYPE_xxx'][size] = key_bytes
    """
    dst = {}  # type: Dict[str, Dict[int, bytes]]
    for typ in src:
        private = "PSA_KEY_TYPE_" + re.sub(r"(\(|\Z)", r"_KEY_PAIR\1", typ, 1)
        public = "PSA_KEY_TYPE_" + re.sub(r"(\(|\Z)", r"_PUBLIC_KEY\1", typ, 1)
        dst[private] = {}
        dst[public] = {}
        for size in src[typ]:
            dst[private][size] = unhexlify(src[typ][size][0])
            dst[public][size] = unhexlify(src[typ][size][1])
    return dst


## These are valid keys that don't try to exercise any edge cases. They're
## either test vectors from some specification, or randomly generated. All
## pairs consist of a private key and its public key.
# pylint: disable=line-too-long
ASYMMETRIC_KEY_DATA = construct_asymmetric_key_data(
    {
        "ECC(PSA_ECC_FAMILY_SECP_K1)": {
            192: (
                "297ac1722ccac7589ecb240dc719842538ca974beb79f228",
                "0426b7bb38da649ac2138fc050c6548b32553dab68afebc36105d325b75538c12323cb0764789ecb992671beb2b6bef2f5",
            ),
            224: (
                "0024122bf020fa113f6c0ac978dfbd41f749257a9468febdbe0dc9f7e8",
                "042cc7335f4b76042bed44ef45959a62aa215f7a5ff0c8111b8c44ed654ee71c1918326ad485b2d599fe2a6eab096ee26d977334d2bac6d61d",
            ),
            256: (
                "7fa06fa02d0e911b9a47fdc17d2d962ca01e2f31d60c6212d0ed7e3bba23a7b9",
                "045c39154579efd667adc73a81015a797d2c8682cdfbd3c3553c4a185d481cdc50e42a0e1cbc3ca29a32a645e927f54beaed14c9dbbf8279d725f5495ca924b24d",
            ),
        },
        "ECC(PSA_ECC_FAMILY_SECP_R1)": {
            225: (
                "872f203b3ad35b7f2ecc803c3a0e1e0b1ed61cc1afe71b189cd4c995",
                "046f00eadaa949fee3e9e1c7fa1247eecec86a0dce46418b9bd3117b981d4bd0ae7a990de912f9d060d6cb531a42d22e394ac29e81804bf160",
            ),
            256: (
                "49c9a8c18c4b885638c431cf1df1c994131609b580d4fd43a0cab17db2f13eee",
                "047772656f814b399279d5e1f1781fac6f099a3c5ca1b0e35351834b08b65e0b572590cdaf8f769361bcf34acfc11e5e074e8426bdde04be6e653945449617de45",
            ),
            384: (
                "3f5d8d9be280b5696cc5cc9f94cf8af7e6b61dd6592b2ab2b3a4c607450417ec327dcdcaed7c10053d719a0574f0a76a",
                "04d9c662b50ba29ca47990450e043aeaf4f0c69b15676d112f622a71c93059af999691c5680d2b44d111579db12f4a413a2ed5c45fcfb67b5b63e00b91ebe59d09a6b1ac2c0c4282aa12317ed5914f999bc488bb132e8342cc36f2ca5e3379c747",
            ),
            521: (
                "01b1b6ad07bb79e7320da59860ea28e055284f6058f279de666e06d435d2af7bda28d99fa47b7dd0963e16b0073078ee8b8a38d966a582f46d19ff95df3ad9685aae",
                "04001de142d54f69eb038ee4b7af9d3ca07736fd9cf719eb354d69879ee7f3c136fb0fbf9f08f86be5fa128ec1a051d3e6c643e85ada8ffacf3663c260bd2c844b6f5600cee8e48a9e65d09cadd89f235dee05f3b8a646be715f1f67d5b434e0ff23a1fc07ef7740193e40eeff6f3bcdfd765aa9155033524fe4f205f5444e292c4c2f6ac1",
            ),
        },
        "ECC(PSA_ECC_FAMILY_SECP_R2)": {
            160: (
                "00bf539a1cdda0d7f71a50a3f98aec0a2e8e4ced1e",
                "049570d541398665adb5cfa16f5af73b3196926bbd4b876bdb80f8eab20d0f540c22f4de9c140f6d7b",
            ),
        },
        "ECC(PSA_ECC_FAMILY_SECT_K1)": {
            163: (
                "03ebc8fcded2d6ab72ec0f75bdb4fd080481273e71",
                "0406f88f90b4b65950f06ce433afdb097e320f433dc2062b8a65db8fafd3c110f46bc45663fbf021ee7eb9",
            ),
            233: (
                "41f08485ce587b06061c087e76e247c359de2ba9927ee013b2f1ed9ca8",
                "0401e9d7189189f773bd8f71be2c10774ba18842434dfa9312595ea545104400f45a9d5675647513ba75b079fe66a29daac2ec86a6a5d4e75c5f290c1f",
            ),
            239: (
                "1a8069ce2c2c8bdd7087f2a6ab49588797e6294e979495602ab9650b9c61",
                "04068d76b9f4508762c2379db9ee8b87ad8d86d9535132ffba3b5680440cfa28eb133d4232faf1c9aba96af11aefe634a551440800d5f8185105d3072d",
            ),
            283: (
                "006d627885dd48b9ec6facb5b3865377d755b75a5d51440e45211c1f600e15eff8a881a0",
                "0405f48374debceaadb46ba385fd92048fcc5b9af1a1c90408bf94a68b9378df1cbfdfb6fb026a96bea06d8f181bf10c020adbcc88b6ecff96bdc564a9649c247cede601c4be63afc3",
            ),
            409: (
                "3ff5e74d932fa77db139b7c948c81e4069c72c24845574064beea8976b70267f1c6f9a503e3892ea1dcbb71fcea423faa370a8",
                "04012c587f69f68b308ba6dcb238797f4e22290ca939ae806604e2b5ab4d9caef5a74a98fd87c4f88d292dd39d92e556e16c6ecc3c019a105826eef507cd9a04119f54d5d850b3720b3792d5d03410e9105610f7e4b420166ed45604a7a1f229d80975ba6be2060e8b",
            ),
            571: (
                "005008c97b4a161c0db1bac6452c72846d57337aa92d8ecb4a66eb01d2f29555ffb61a5317225dcc8ca6917d91789e227efc0bfe9eeda7ee21998cd11c3c9885056b0e55b4f75d51",
                "04050172a7fd7adf98e4e2ed2742faa5cd12731a15fb0dbbdf75b1c3cc771a4369af6f2fa00e802735650881735759ea9c79961ded18e0daa0ac59afb1d513b5bbda9962e435f454fc020b4afe1445c2302ada07d295ec2580f8849b2dfa7f956b09b4cbe4c88d3b1c217049f75d3900d36df0fa12689256b58dd2ef784ebbeb0564600cf47a841485f8cf897a68accd5a",
            ),
        },
        "ECC(PSA_ECC_FAMILY_SECT_R1)": {
            163: (
                "009b05dc82d46d64a04a22e6e5ca70ca1231e68c50",
                "0400465eeb9e7258b11e33c02266bfe834b20bcb118700772796ee4704ec67651bd447e3011959a79a04cb",
            ),
            233: (
                "00e5e42834e3c78758088b905deea975f28dc20ef6173e481f96e88afe7f",
                "0400cd68c8af4430c92ec7a7048becfdf00a6bae8d1b4c37286f2d336f2a0e017eca3748f4ad6d435c85867aa014eea1bd6d9d005bbd8319cab629001d",
            ),
            283: (
                "004cecad915f6f3c9bbbd92d1eb101eda23f16c7dad60a57c87c7e1fd2b29b22f6d666ad",
                "04052f9ff887254c2d1440ba9e30f13e2185ba53c373b2c410dae21cf8c167f796c08134f601cbc4c570bffbc2433082cf4d9eb5ba173ecb8caec15d66a02673f60807b2daa729b765",
            ),
            409: (
                "00c22422d265721a3ae2b3b2baeb77bee50416e19877af97b5fc1c700a0a88916ecb9050135883accb5e64edc77a3703f4f67a64",
                "0401aa25466b1d291846db365957b25431591e50d9c109fe2106e93bb369775896925b15a7bfec397406ab4fe6f6b1a13bf8fdcb9300fa5500a813228676b0a6c572ed96b0f4aec7e87832e7e20f17ca98ecdfd36f59c82bddb8665f1f357a73900e827885ec9e1f22",
            ),
            571: (
                "026ac1cdf92a13a1b8d282da9725847908745138f5c6706b52d164e3675fcfbf86fc3e6ab2de732193267db029dd35a0599a94a118f480231cfc6ccca2ebfc1d8f54176e0f5656a1",
                "040708f3403ee9948114855c17572152a08f8054d486defef5f29cbffcfb7cfd9280746a1ac5f751a6ad902ec1e0525120e9be56f03437af196fbe60ee7856e3542ab2cf87880632d80290e39b1a2bd03c6bbf6225511c567bd2ff41d2325dc58346f2b60b1feee4dc8b2af2296c2dc52b153e0556b5d24152b07f690c3fa24e4d1d19efbdeb1037833a733654d2366c74",
            ),
        },
        "ECC(PSA_ECC_FAMILY_SECT_R2)": {
            163: (
                "0210b482a458b4822d0cb21daa96819a67c8062d34",
                "0403692601144c32a6cfa369ae20ae5d43c1c764678c037bafe80c6fd2e42b7ced96171d9c5367fd3dca6f",
            ),
        },
        "ECC(PSA_ECC_FAMILY_BRAINPOOL_P_R1)": {
            160: (
                "69502c4fdaf48d4fa617bdd24498b0406d0eeaac",
                "04d4b9186816358e2f9c59cf70748cb70641b22fbab65473db4b4e22a361ed7e3de7e8a8ddc4130c5c",
            ),
            192: (
                "1688a2c5fbf4a3c851d76a98c3ec88f445a97996283db59f",
                "043fdd168c179ff5363dd71dcd58de9617caad791ae0c37328be9ca0bfc79cebabf6a95d1c52df5b5f3c8b1a2441cf6c88",
            ),
            224: (
                "a69835dafeb5da5ab89c59860dddebcfd80b529a99f59b880882923c",
                "045fbea378fc8583b3837e3f21a457c31eaf20a54e18eb11d104b3adc47f9d1c97eb9ea4ac21740d70d88514b98bf0bc31addac1d19c4ab3cc",
            ),
            256: (
                "2161d6f2db76526fa62c16f356a80f01f32f776784b36aa99799a8b7662080ff",
                "04768c8cae4abca6306db0ed81b0c4a6215c378066ec6d616c146e13f1c7df809b96ab6911c27d8a02339f0926840e55236d3d1efbe2669d090e4c4c660fada91d",
            ),
            320: (
                "61b8daa7a6e5aa9fccf1ef504220b2e5a5b8c6dc7475d16d3172d7db0b2778414e4f6e8fa2032ead",
                "049caed8fb4742956cc2ad12a9a1c995e21759ef26a07bc2054136d3d2f28bb331a70e26c4c687275ab1f434be7871e115d2350c0c5f61d4d06d2bcdb67f5cb63fdb794e5947c87dc6849a58694e37e6cd",
            ),
            384: (
                "3dd92e750d90d7d39fc1885cd8ad12ea9441f22b9334b4d965202adb1448ce24c5808a85dd9afc229af0a3124f755bcb",
                "04719f9d093a627e0d350385c661cebf00c61923566fe9006a3107af1d871bc6bb68985fd722ea32be316f8e783b7cd1957785f66cfc0cb195dd5c99a8e7abaa848553a584dfd2b48e76d445fe00dd8be59096d877d4696d23b4bc8db14724e66a",
            ),
            512: (
                "372c9778f69f726cbca3f4a268f16b4d617d10280d79a6a029cd51879fe1012934dfe5395455337df6906dc7d6d2eea4dbb2065c0228f73b3ed716480e7d71d2",
                "0438b7ec92b61c5c6c7fbc28a4ec759d48fcd4e2e374defd5c4968a54dbef7510e517886fbfc38ea39aa529359d70a7156c35d3cbac7ce776bdb251dd64bce71234424ee7049eed072f0dbc4d79996e175d557e263763ae97095c081e73e7db2e38adc3d4c9a0487b1ede876dc1fca61c902e9a1d8722b8612928f18a24845591a",
            ),
        },
        "ECC(PSA_ECC_FAMILY_MONTGOMERY)": {
            255: (
                "70076d0a7318a57d3c16c17251b26645df4c2f87ebc0992ab177fba51db92c6a",
                "8520f0098930a754748b7ddcb43ef75a0dbf3a0d26381af4eba4a98eaa9b4e6a",
            ),
            448: (
                "e4e49f52686f9ee3b638528f721f1596196ffd0a1cddb64c3f216f06541805cfeb1a286dc78018095cdfec050e8007b5f4908962ba20d6c1",
                "c0d3a5a2b416a573dc9909f92f134ac01323ab8f8e36804e578588ba2d09fe7c3e737f771ca112825b548a0ffded6d6a2fd09a3e77dec30e",
            ),
        },
        "ECC(PSA_ECC_FAMILY_TWISTED_EDWARDS)": {
            255: (
                "9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60",
                "d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a",
            ),
            448: (
                "6c82a562cb808d10d632be89c8513ebf6c929f34ddfa8c9f63c9960ef6e348a3528c8a3fcc2f044e39a3fc5b94492f8f032e7549a20098f95b",
                "5fd7449b59b461fd2ce787ec616ad46a1da1342485a70e1f8a0ea75d80e96778edf124769b46c7061bd6783df1e50f6cd1fa1abeafe8256180",
            ),
        },
        "RSA": {
            1024: (
                """
3082025e
 020100
 02818100af057d396ee84fb75fdbb5c2b13c7fe5a654aa8aa2470b541ee1feb0b12d25c79711531249e1129628042dbbb6c120d1443524ef4c0e6e1d8956eeb2077af12349ddeee54483bc06c2c61948cd02b202e796aebd94d3a7cbf859c2c1819c324cb82b9cd34ede263a2abffe4733f077869e8660f7d6834da53d690ef7985f6bc3
 0203010001
 02818100874bf0ffc2f2a71d14671ddd0171c954d7fdbf50281e4f6d99ea0e1ebcf82faa58e7b595ffb293d1abe17f110b37c48cc0f36c37e84d876621d327f64bbe08457d3ec4098ba2fa0a319fba411c2841ed7be83196a8cdf9daa5d00694bc335fc4c32217fe0488bce9cb7202e59468b1ead119000477db2ca797fac19eda3f58c1
 024100e2ab760841bb9d30a81d222de1eb7381d82214407f1b975cbbfe4e1a9467fd98adbd78f607836ca5be1928b9d160d97fd45c12d6b52e2c9871a174c66b488113
 024100c5ab27602159ae7d6f20c3c2ee851e46dc112e689e28d5fcbbf990a99ef8a90b8bb44fd36467e7fc1789ceb663abda338652c3c73f111774902e840565927091
 024100b6cdbd354f7df579a63b48b3643e353b84898777b48b15f94e0bfc0567a6ae5911d57ad6409cf7647bf96264e9bd87eb95e263b7110b9a1f9f94acced0fafa4d
 024071195eec37e8d257decfc672b07ae639f10cbb9b0c739d0c809968d644a94e3fd6ed9287077a14583f379058f76a8aecd43c62dc8c0f41766650d725275ac4a1
 024100bb32d133edc2e048d463388b7be9cb4be29f4b6250be603e70e3647501c97ddde20a4e71be95fd5e71784e25aca4baf25be5738aae59bbfe1c997781447a2b24
""",
                """
 308189
  02818100af057d396ee84fb75fdbb5c2b13c7fe5a654aa8aa2470b541ee1feb0b12d25c79711531249e1129628042dbbb6c120d1443524ef4c0e6e1d8956eeb2077af12349ddeee54483bc06c2c61948cd02b202e796aebd94d3a7cbf859c2c1819c324cb82b9cd34ede263a2abffe4733f077869e8660f7d6834da53d690ef7985f6bc3
 0203010001
""",
            ),
            1536: (
                """
3082037b
 020100
 0281c100c870feb6ca6b1d2bd9f2dd99e20f1fe2d7e5192de662229dbe162bd1ba66336a7182903ca0b72796cd441c83d24bcdc3e9a2f5e4399c8a043f1c3ddf04754a66d4cfe7b3671a37dd31a9b4c13bfe06ee90f9d94ddaa06de67a52ac863e68f756736ceb014405a6160579640f831dddccc34ad0b05070e3f9954a58d1815813e1b83bcadba814789c87f1ef2ba5d738b793ec456a67360eea1b5faf1c7cc7bf24f3b2a9d0f8958b1096e0f0c335f8888d0c63a51c3c0337214fa3f5efdf6dcc35
 0203010001
 0281c06d2d670047973a87752a9d5bc14f3dae00acb01f593aa0e24cf4a49f932931de4bbfb332e2d38083da80bc0b6d538edba479f7f77d0deffb4a28e6e67ff6273585bb4cd862535c946605ab0809d65f0e38f76e4ec2c3d9b8cd6e14bcf667943892cd4b34cc6420a439abbf3d7d35ef73976dd6f9cbde35a51fa5213f0107f83e3425835d16d3c9146fc9e36ce75a09bb66cdff21dd5a776899f1cb07e282cca27be46510e9c799f0d8db275a6be085d9f3f803218ee3384265bfb1a3640e8ca1
 026100e6848c31d466fffefc547e3a3b0d3785de6f78b0dd12610843512e495611a0675509b1650b27415009838dd8e68eec6e7530553b637d602424643b33e8bc5b762e1799bc79d56b13251d36d4f201da2182416ce13574e88278ff04467ad602d9
 026100de994fdf181f02be2bf9e5f5e4e517a94993b827d1eaf609033e3a6a6f2396ae7c44e9eb594cf1044cb3ad32ea258f0c82963b27bb650ed200cde82cb993374be34be5b1c7ead5446a2b82a4486e8c1810a0b01551609fb0841d474bada802bd
 026076ddae751b73a959d0bfb8ff49e7fcd378e9be30652ecefe35c82cb8003bc29cc60ae3809909baf20c95db9516fe680865417111d8b193dbcf30281f1249de57c858bf1ba32f5bb1599800e8398a9ef25c7a642c95261da6f9c17670e97265b1
 0260732482b837d5f2a9443e23c1aa0106d83e82f6c3424673b5fdc3769c0f992d1c5c93991c7038e882fcda04414df4d7a5f4f698ead87851ce37344b60b72d7b70f9c60cae8566e7a257f8e1bef0e89df6e4c2f9d24d21d9f8889e4c7eccf91751
 026009050d94493da8f00a4ddbe9c800afe3d44b43f78a48941a79b2814a1f0b81a18a8b2347642a03b27998f5a18de9abc9ae0e54ab8294feac66dc87e854cce6f7278ac2710cb5878b592ffeb1f4f0a1853e4e8d1d0561b6efcc831a296cf7eeaf
""",
                """
3081c9
 0281c100c870feb6ca6b1d2bd9f2dd99e20f1fe2d7e5192de662229dbe162bd1ba66336a7182903ca0b72796cd441c83d24bcdc3e9a2f5e4399c8a043f1c3ddf04754a66d4cfe7b3671a37dd31a9b4c13bfe06ee90f9d94ddaa06de67a52ac863e68f756736ceb014405a6160579640f831dddccc34ad0b05070e3f9954a58d1815813e1b83bcadba814789c87f1ef2ba5d738b793ec456a67360eea1b5faf1c7cc7bf24f3b2a9d0f8958b1096e0f0c335f8888d0c63a51c3c0337214fa3f5efdf6dcc35
 0203010001
""",
            ),
        },
    }
)
