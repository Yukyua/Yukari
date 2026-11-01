#include "yukari/translation_patch.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>

int main() {
    const auto directory=std::filesystem::temp_directory_path()/"yukari-translation-patch-test";
    const auto path=directory/"translation_patch";
    const auto explicit_path=directory/"explicit_patch";
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory);
    {
        std::ofstream out(path);
        out<<"YUKARI_TRANSLATION_PATCH_1\n"
              "@encoding\tCP936\n"
              "594b535c53544f52595c464958545552452e594b53\t2a\te38184e3828de381af\te5bda9e58d8e\n";
    }
    yukari::TranslationPatch patch(path);
    assert(patch.size()==1);
    assert(patch.encoding()&&*patch.encoding()=="CP936");
    assert(yukari::find_translation_patch(directory)==path);
    assert(yukari::find_translation_patch(directory,explicit_path)==explicit_path);
    assert(yukari::find_translation_patch(directory/"missing").empty());
    yukari::Script script;
    yukari::Element literal{};literal.type=5;literal.b=0x2a;literal.text="いろは";
    script.elements.push_back(literal);
    assert(patch.verify(script,"YKS\\STORY\\FIXTURE.YKS")==1);
    const auto translated=patch.apply(script,"yks/story/fixture.yks");
    assert(translated.elements[0].text=="彩华");
    script.elements[0].text="different";
    bool mismatch=false;
    try { patch.apply(script,"YKS\\STORY\\FIXTURE.YKS"); }
    catch(const std::runtime_error&) { mismatch=true; }
    assert(mismatch);
    {
        std::ofstream out(explicit_path);
        out<<"YUKARI_TRANSLATION_PATCH_1\n";
    }
    yukari::TranslationPatch legacy(explicit_path);
    assert(!legacy.encoding());
    std::filesystem::remove_all(directory);
}
