#!/usr/bin/env python3
"""Synthetic host/VM integration regressions; never reads or changes a real game.
Run: python3 tests/runtime_commands.py --exe build/yukari
Media cases use local ffmpeg to generate a one-second MPEG and short OGG.
"""
import argparse
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import struct
import tempfile
import unittest
from runtime_fixtures import Script,archive,png,read_png

EXE=None
class RuntimeCommands(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory(prefix='yukari-runtime-commands-');self.root=Path(self.tmp.name);self.game=self.root/'game';self.game.mkdir();self.state=self.root/'state'
    def tearDown(self):self.tmp.cleanup()
    def package(self,start,extra=()):
        files=[('start.yks',start.finish()),('images\\base.png',png()),*extra]
        (self.game/'data01.ykc').write_bytes(archive(files))
    def run_game(self,actions=(),max_ms=60000,extra=()):
        before={p.relative_to(self.game):hashlib.sha256(p.read_bytes()).hexdigest() for p in self.game.rglob('*') if p.is_file()}
        script=self.root/'actions.txt';script.write_text('\n'.join(actions)+'\n')
        env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy',SDL_RENDER_DRIVER='software')
        p=subprocess.run([str(EXE),'play',str(self.game),'--fast','--mute','--seed','0','--state-dir',str(self.state),'--actions',str(script),'--trace','--max-ms',str(max_ms),*extra],env=env,capture_output=True,text=True,timeout=30)
        log=p.stdout+p.stderr
        self.assertEqual(p.returncode,0,log[-9000:])
        after={p.relative_to(self.game):hashlib.sha256(p.read_bytes()).hexdigest() for p in self.game.rglob('*') if p.is_file()}
        self.assertEqual(before,after,'runtime changed synthetic source packages/media')
        flags={}
        for line in (self.state/'yukari-system.dat').read_text().splitlines():
            parts=line.split()
            if parts and parts[0]=='F':flags[int(parts[1])]=int(parts[2])
        return flags,log
    def base(self):
        s=Script();s.call('DefaultFontSet',0,'Synthetic',24,0,24,30);s.call('TextWindowSet',0,100,100,500,300);s.call('FontSet',0,0);return s
    def end(self,s):s.call('SystemDataWrite');s.call('ScriptExit')
    def icon_executable(self):
        # Synthetic PE32 with one group icon and PNG image; no original EXE bytes.
        image=png(2,2)
        group=struct.pack('<HHHBBBBHHIH',0,1,1,2,2,0,0,1,32,len(image),1)
        pe=bytearray(0x400+len(image))
        def w(at,value):struct.pack_into('<H',pe,at,value)
        def d(at,value):struct.pack_into('<I',pe,at,value)
        w(0,0x5a4d);d(0x3c,0x80);d(0x80,0x4550);w(0x86,1);w(0x94,224)
        w(0x98,0x10b);d(0x98+92,16);d(0x98+112,0x1000)
        d(0x178+12,0x1000);d(0x178+16,len(pe)-0x200);d(0x178+20,0x200)
        w(0x20e,2)
        for index,(kind,blob,offset) in enumerate(((3,image,0x400),(14,group,0x300))):
            directory=0x20+index*0x60
            d(0x210+index*8,kind);d(0x214+index*8,0x80000000|directory)
            w(0x200+directory+14,1);d(0x200+directory+16,1)
            d(0x200+directory+20,0x80000000|(directory+24))
            w(0x200+directory+24+14,1);d(0x200+directory+24+16,1041)
            d(0x200+directory+24+20,directory+48)
            d(0x200+directory+48,0x1000+offset-0x200)
            d(0x200+directory+52,len(blob));pe[offset:offset+len(blob)]=blob
        return pe
    def test_window_icon_uses_directory_named_executable(self):
        (self.game/'game.exe').write_bytes(self.icon_executable())
        (self.game/'patch.exe').write_bytes(b'not an original executable')
        s=Script();s.call('WindowIconChange',1);self.end(s);self.package(s)
        self.run_game()
    def test_window_icon_explicit_executable_overrides_default(self):
        (self.game/'game.exe').write_bytes(b'invalid default executable')
        original=self.game/'original.exe';original.write_bytes(self.icon_executable())
        s=Script();s.call('WindowIconChange',1);self.end(s);self.package(s)
        self.run_game(extra=('--original-exe',str(original)))
    def test_flag_reset_preserves_global_state(self):
        s=self.base();s.assign('Flag',500,73);s.assign('String',501,'OLD')
        s.assign('GlobalFlag',9000,17);s.assign('GlobalString',9003,'KEEP');s.call('FlagReset')
        s.code.append(s.bank('GlobalFlag',9001));s.call('=',0);s.code[-1]=s.bank('Flag',500)
        s.code.append(s.bank('GlobalString',9002));s.call('=','');s.code[-1]=s.bank('String',501)
        s.call('SaveDataSave',2,'RESET');self.end(s);self.package(s)
        flags,log=self.run_game();self.assertEqual(flags.get(9000),17,log);self.assertEqual(flags.get(9001,0),0,log)
        system=(self.state/'yukari-system.dat').read_text();self.assertIn('KEEP',system);self.assertNotIn('OLD',system)
        self.assertLess((self.state/'savedata.002.yukari').stat().st_size,32768,'empty banks expanded VM checkpoint')

    def test_text_lf_color_filters(self):
        s=self.base();s.call('GraphicLoad',1,'images\\base.png',10,10);s.call('StrOutNW',0,'FIRST');s.call('LF',0);s.call('StrOutNW',0,'SECOND');s.call('KeyWait')
        s.call('GraphicAddColor',1,0x000a0014);s.call('GraphicSubColor',1,0x00000500);s.call('KeyWait');s.assign('GlobalFlag',9000,1);self.end(s);self.package(s)
        original=self.root/'before.png';changed=self.root/'after.png';flags,log=self.run_game([f'capture {original}','advance',f'capture {changed}','advance'])
        self.assertEqual(flags.get(9000),1,log);w,h,pixel=read_png(original);self.assertEqual((w,h),(1024,576));self.assertEqual(pixel(20,20),(40,60,80));self.assertEqual(read_png(changed)[2](20,20),(60,55,90))
        self.assertTrue(any(max(pixel(x,y))>160 for y in range(100,130) for x in range(100,220)),'first text line absent')
        self.assertTrue(any(max(pixel(x,y))>160 for y in range(130,165) for x in range(100,220)),'LF did not place second line below first')
    def test_colon_separated_graphics_compose_in_order(self):
        s=Script();s.call('GraphicLoad',1,'images\\base.png:images\\overlay.png',10,10);s.call('KeyWait');self.end(s)
        self.package(s,[('images\\overlay.png',png(color=(220,10,20,128)))])
        capture=self.root/'composite.png';flags,log=self.run_game([f'capture {capture}','advance'])
        pixel=read_png(capture)[2](12,12)
        self.assertTrue(all(abs(a-b)<=2 for a,b in zip(pixel,(130,35,50))),f'colon graphic composition differs: {pixel}\n{log}')
    def test_text_select_click(self):
        s=self.base();s.call('SelectMode',0);s.call('SelectPosition',0,100,100,300,40);s.call('Select')
        def choices(p):
            p.call('Case','LEFT',100,100);p.block(lambda b:b.assign('GlobalFlag',9000,11));p.call('Case','RIGHT',100,200);p.block(lambda b:b.assign('GlobalFlag',9000,12))
        s.block(choices);s.assign('GlobalFlag',9001,1);self.end(s);self.package(s);flags,log=self.run_game(['click 120 110']);self.assertEqual((flags.get(9000),flags.get(9001)),(11,1),log)
    def test_mouse_selection_cancel(self):
        s=Script();s.call('MouseSelect')
        def choices(p):
            p.call('MouseCase',1,0,0,50,50);p.block(lambda b:b.assign('GlobalFlag',9000,33));p.call('MouseKeyCase',2);p.block(lambda b:b.assign('GlobalFlag',9000,44))
        s.block(choices);self.end(s);self.package(s);flags,log=self.run_game(['cancel']);self.assertEqual(flags.get(9000),44,log)
    def test_timed_selection_interrupt_and_tte(self):
        s=self.base();s.call('SelectTTE')
        def choices(p):
            p.call('TimeInterrupt',32);p.block(lambda b:b.assign('GlobalFlag',9010,7));p.call('Case','CHOOSE',100,100);p.block(lambda b:b.assign('GlobalFlag',9000,99))
        s.block(choices);s.call('TTE',200);s.block(lambda b:b.assign('GlobalFlag',9000,22));s.call('TTE',1);s.block(lambda b:b.assign('GlobalFlag',9002,99));self.end(s);self.package(s)
        flags,log=self.run_game(['wait-ms 96','click 120 110']);self.assertEqual((flags.get(9000),flags.get(9010),flags.get(9002,0)),(22,7,0),log)
    def test_subcall_checkpoint_and_return(self):
        s=Script();s.assign('GlobalFlag',9000,1);s.call('ScriptSubCall','menu.yks');s.assign('GlobalFlag',9000,3);self.end(s)
        menu=Script();menu.assign('GlobalFlag',9001,2);menu.call('Return');self.package(s,[('menu.yks',menu.finish())]);flags,log=self.run_game()
        self.assertEqual((flags.get(9000),flags.get(9001)),(3,2),log);self.assertTrue((self.state/'savedata.---').read_bytes().startswith(b'Yuka'),'native subcall checkpoint absent');self.assertTrue((self.state/'savedata.---.yukari').is_file())
    def test_actions_save_restore_waiting_page(self):
        s=self.base();s.call('StrOutNW',0,'PAGE1');s.call('KeyWait');s.assign('GlobalFlag',9000,1);s.call('PF',0);s.call('StrOutNW',0,'PAGE2');s.call('KeyWait');s.assign('GlobalFlag',9000,2);self.end(s);self.package(s)
        flags,log=self.run_game(['save 7','advance','load 7','advance','advance']);self.assertEqual(flags.get(9000),2,log);self.assertIn('SAVE_READY slot=7',log);self.assertIn('LOAD_READY slot=7',log);self.assertGreaterEqual(log.count('TEXT_READY'),3,log)
    @unittest.skipUnless(shutil.which('ffmpeg'),'ffmpeg fixture generator is unavailable')
    def test_generated_mpeg_audio_and_synchronous_return(self):
        movie=self.game/'avi'/'fixture.mpg';movie.parent.mkdir();ogg=self.root/'tone.ogg'
        subprocess.run(['ffmpeg','-v','error','-f','lavfi','-i','testsrc2=size=64x48:rate=25','-f','lavfi','-i','sine=frequency=440:sample_rate=44100','-t','1','-c:v','mpeg1video','-bf','2','-c:a','mp2','-ar','44100','-y',str(movie)],check=True)
        subprocess.run(['ffmpeg','-v','error','-f','lavfi','-i','sine=frequency=880:sample_rate=44100','-t','0.1','-ac','2','-c:a','vorbis','-strict','-2','-y',str(ogg)],check=True)
        s=Script();s.call('SoundLoad',5,'tone.ogg');s.call('SoundPlay',5);s.call('Wait',100);s.call('SoundStop',5);s.call('MovieVolumeSet',500);s.call('MoviePlay','avi\\fixture.mpg');s.assign('GlobalFlag',9000,77);self.end(s);self.package(s,[('tone.ogg',ogg.read_bytes())]);flags,log=self.run_game();self.assertEqual(flags.get(9000),77,log);self.assertIn('MoviePlay',log)

    def test_native_metadata_copy_and_delete(self):
        s=Script();s.call('Index','CHAPTER');s.call('SaveDataSave',3,'USER');s.call('SaveDataCopy',3,4);s.call('SaveDataCommentChange',4,'EDITED');s.code.append(s.bank('GlobalString',9000));s.call('SaveDataCommentGet',4);s.call('SaveDataDelete',3);self.end(s);self.package(s);flags,log=self.run_game()
        self.assertFalse((self.state/'savedata.003').exists());self.assertFalse((self.state/'savedata.003.yukari').exists());self.assertIn('EDITED',(self.state/'yukari-system.dat').read_text())
        import struct
        raw=(self.state/'savedata.004').read_bytes()
        def comment(field):
            offset,length=struct.unpack_from('<II',raw,field);return raw[offset:offset+length].split(b'\0')[0].decode('cp932')
        self.assertEqual((comment(0x10),comment(0x18)),('EDITED','CHAPTER'))
    def test_subcall_save_restores_parent_continuation(self):
        s=self.base();s.call('ScriptSubCall','menu.yks');s.call('KeyWait');self.end(s)
        menu=Script();menu.call('SaveDataSave',7,'MENU');menu.call('Return');self.package(s,[('menu.yks',menu.finish())]);flags,log=self.run_game(['load 7','advance'])
        self.assertEqual(log.count('  ScriptSubCall'),1,log);self.assertEqual(log.count('  SaveDataSave'),1,log)
    def test_text_targets_round_robin_even_when_immediate(self):
        s=self.base();s.call('StrOutNW',0,'AB',0,'xy');s.code.append(s.bank('GlobalString',9017));s.call('StrIn',0);self.end(s);self.package(s);flags,log=self.run_game()
        self.assertIn('AxBy',(self.state/'yukari-system.dat').read_text())
    def test_keywait_function_mask_gates_history_key(self):
        s=self.base();s.call('GraphicLoad',0,'images\\base.png',0,0);s.call('SystemKeySetHistoryMode',65);s.call('StrOutNW',0,'ONE');s.call('KeyWait');s.call('PF');s.call('KeyWaitFunctionSet',0);s.call('StrOutNW',0,'TWO');s.call('KeyWait');s.call('KeyWaitFunctionSet',0x201);s.call('KeyWait');self.end(s);self.package(s)
        flags,log=self.run_game(['advance','key A','wait-ms 64','advance','key A','wait-ms 64','history-close','advance'])
        self.assertEqual(log.count('HISTORY_READY'),1,log)

    def test_auto_button_second_click_cancels_and_consumes(self):
        s=self.base();s.call('GraphicLoad',13,'images\\base.png',0,0)
        s.call('AreaSetAutoMode',13,10,10,60,40);s.call('KeyWaitFunctionSet',0x10)
        s.call('AutoModeCancelLKey',1);s.call('AutoSpeedSet',100000)
        s.call('StrOutNW',0,'AUTO');s.call('KeyWait');self.end(s);self.package(s)
        flags,log=self.run_game(['click 20 20','click 20 20','quit'])
        self.assertIn('AUTO_MODE value=1 reason=area',log)
        self.assertIn('AUTO_MODE value=0 reason=left-click-cancel',log)
        self.assertNotIn('ScriptExit',log,'cancel click advanced instead of repeating KeyWait')

    def test_load_hook_runs_above_restored_context(self):
        s=self.base();s.call('SaveDataLoadToCallScript','load-hook.yks');s.call('StrOutNW',0,'PARENT');s.call('KeyWait');s.assign('GlobalFlag',9000,18);s.call('KeyWait');self.end(s)
        hook=Script();hook.assign('GlobalFlag',9011,77);hook.call('GraphicRefresh');hook.code.append(hook.bank('GlobalFlag',9012));hook.call('NowScriptStateGet');hook.call('Return')
        self.package(s,[('load-hook.yks',hook.finish())]);flags,log=self.run_game(['save 7','advance','load 7','advance','advance'])
        self.assertEqual((flags.get(9000),flags.get(9011),flags.get(9012,0)),(18,77,0),log)
        self.assertIn('load-hook.yks:word=',log)

    def test_native_load_hook_keeps_new_parent_wait(self):
        s=self.base();s.call('SaveDataLoadToCallScript','load-hook.yks');s.call('StrOutNW',0,'PARENT');s.call('KeyWait');s.assign('GlobalFlag',9000,18);s.call('KeyWait');self.end(s)
        hook=Script();hook.assign('GlobalFlag',9011,77);hook.call('GraphicRefresh');hook.call('Return');self.package(s,[('load-hook.yks',hook.finish())]);flags,log=self.run_game(['save 7','advance','load-original 7','advance','advance'])
        self.assertEqual((flags.get(9000),flags.get(9011)),(18,77),log)
        self.assertIn('LOAD_ORIGINAL_READY slot=7',log)

    def test_save_preview_configuration_parent_and_snapshot(self):
        s=self.base();s.call('SaveDataSave',3,'USER');s.call('SaveDataThumbnailChange',3,'images\\base.png');s.call('SaveDataDisplaySetToGraphic',5,6,16,16);s.call('SaveDataDisplaySetToDate');s.call('SaveDataDisplaySetToComment1');s.call('SaveDataDisplaySetToComment2')
        s.code.append(s.bank('GlobalFlag',9015));s.call('GraphicCreateSaveData',20,3,100,100);s.call('KeyWait');s.call('GraphicMove',20,200,100);s.call('KeyWait');s.call('GraphicHide',20);s.call('KeyWait');self.end(s);self.package(s)
        first=self.root/'preview.png';restored=self.root/'preview-restored.png';moved=self.root/'preview-moved.png';hidden=self.root/'preview-hidden.png'
        flags,log=self.run_game([f'capture {first}','save 7','load 7',f'capture {restored}','advance',f'capture {moved}','advance',f'capture {hidden}','advance'])
        self.assertEqual(flags.get(9015),1,log);self.assertEqual(first.read_bytes(),restored.read_bytes());self.assertEqual(read_png(first)[2](110,110),(40,60,80));self.assertEqual(read_png(moved)[2](110,110),(0,0,0));self.assertEqual(read_png(moved)[2](210,110),(40,60,80));self.assertEqual(read_png(hidden)[2](210,110),(0,0,0))

    def test_save_preview_text_metrics_and_snapshot(self):
        s=self.base();s.call('SaveDataSave',3,'日本語 ABC');s.call('SaveDataDisplaySetToGraphic');s.call('SaveDataDisplaySetToFont',0,4,0xffffff);s.call('SaveDataDisplaySetToComment1',20,40);s.call('SaveDataDisplaySetToComment2');s.call('GraphicCreateSaveData',20,3,100,100);s.call('KeyWait');self.end(s);self.package(s)
        first=self.root/'preview-text.png';restored=self.root/'preview-text-restored.png'
        flags,log=self.run_game([f'capture {first}','save 7','load 7',f'capture {restored}','advance'])
        self.assertEqual(first.read_bytes(),restored.read_bytes());w,h,pixel=read_png(first)
        self.assertTrue(any(max(pixel(x,y))>180 for y in range(140,175) for x in range(120,360)),'save preview comment glyphs absent')

    def test_copy_pixels_survive_save_and_swap(self):
        s=self.base();s.call('GraphicLoad',1,'images\\base.png',10,10);s.call('GraphicLoad',2,'red.png',100,10);s.call('GraphicCopy2',1,0,0,2,0,0,16,16);s.call('GraphicSwap',1,2);s.call('KeyWait');self.end(s);self.package(s,[('red.png',png(color=(220,10,20,255)))])
        before=self.root/'copy.png';after=self.root/'copy-reload.png';flags,log=self.run_game([f'capture {before}','save 7','load 7',f'capture {after}','advance'])
        self.assertEqual(before.read_bytes(),after.read_bytes(),'GraphicCopy pixels were reconstructed from unchanged source asset');self.assertEqual(read_png(before)[2](12,12),(220,10,20))

    def test_selection_key_history_save_restore(self):
        s=Script();s.call('MouseSelect');s.block(lambda b:(b.call('KeyTableCase',65,66),b.block(lambda c:c.assign('GlobalFlag',9000,82))));s.call('KeyWait');self.end(s);self.package(s)
        flags,log=self.run_game(['key A','save 7','key B','load 7','key B','advance']);self.assertEqual(flags.get(9000),82,log);self.assertIn('LOAD_READY',log)
    def test_window_message_case_dispatch(self):
        s=Script();s.call('MouseSelect');s.block(lambda b:(b.call('WindowMessageCase',0x101),b.block(lambda c:c.assign('GlobalFlag',9000,91))));self.end(s);self.package(s)
        flags,log=self.run_game(['key Z']);self.assertEqual(flags.get(9000),91,log)

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--exe',type=Path,required=True);args,rest=parser.parse_known_args();EXE=args.exe.resolve();unittest.main(argv=[__file__,*rest])
