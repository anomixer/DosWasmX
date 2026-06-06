# DOS Wasm X — AI Agents 開發與優化歷程記錄

此文件記錄了 AI Agents 在協助將 Roland MT-32 模擬整合進 `DosWasmX` 專案中的開發、除錯與效能優化歷程。

---

## 🚀 專案目標與背景

本專案旨在將 **Roland MT-32 音效模擬器（Munt/mt32emu 函式庫）** 整合進 Emscripten 移植版的 DOSBox-X 模擬器中，讓使用者能直接在網頁端（WebAssembly）播放高品質的 MT-32 MIDI 遊戲配樂。

---

## 🛠️ 開發與優化歷程

AI Agents 主要參與並解決了以下各階段的挑戰：

### 階段 1：MT-32 核心代碼整合與音訊掛載
- **核心整合**：下載 Munt/mt32emu 完整原始碼，將其置於 `code/src/libs/mt32/`，並加入手寫的 `code/Makefile` 中，採用內建的重新採樣器（`InternalResampler`）以避免對 Emscripten 尚未移植的 `libsoxr`/`libsamplerate` 之相依性。
- **ROM 檔案自動載入**：
  - 修改 `dist/script.js`，在模擬器啟動前自動透過 `fetch` 載入 `MT32_CONTROL.ROM` 與 `MT32_PCM.ROM`，並透過 Emscripten 的 `FS.writeFile` 寫入虛擬檔案系統（VFS）的根目錄 `/` 下。
  - 調整 `dist/dosbox-x-for-web.conf` 檔案，設定 `mididevice=mt32` 以及 `mt32.romdir=.`，確保 DOSBox-X 能正確抓取並初始化 Munt 模擬。

---

### 階段 2：解決 Runtime `unreachable` 崩潰與連結問題
這是移植過程中遇到的最大技術瓶頸，其成因與修復如下：

#### 1. JavaScript Exceptions 與 Asyncify 衝突（Runtime 崩潰）
- **現象**：若在 Makefile 中使用傳統 JavaScript 例外處理 `-fexceptions`，當網頁執行 DOS 指令（例如自動執行的 `XCOPY`）時，網頁會拋出 `Uncaught RuntimeError: unreachable` 並卡死。
- **成因**：DOSBox-X 中許多 CPU 回圈與 Page Fault 處理均高度依賴 C++ 例外機制。在 JS-based 例外模式下，Emscripten 的 try-catch 包裝函式在與 `ASYNCIFY`（用於實作 JS 睡眠暫停與喚醒）的 stack unwind/rewind 交互時，會使執行緒狀態混亂，導致恢復執行時走入 unreachable 分支。
- **解決方案**：必須強制使用 WebAssembly 原生異常處理選項 `-fwasm-exceptions`。

#### 2. Wasm Exceptions 與 Asyncify 在 wasm-opt 中的相容性錯誤（連結失敗）
- **現象**：啟用 `-fwasm-exceptions` 進行連結時，`emsdk` 中的 `wasm-opt` 會報出 fatal error：`__asyncify_get_call_index does not exist`。這是 Binaryen 對原生 Exception 與 Asyncify 同時啟用時的已知 bug。
- **解決方案**：
  - 原作者在 `code/` 下提供了一個特製的 Linux 版本 `wasm-opt`。由於 Windows 宿主機無法直接執行此 ELF 二進位檔，我們轉為使用 **WSL (Windows Subsystem for Linux - Ubuntu)** 環境，並安裝 `emsdk 3.1.49`。
  - 將專案特製的 `wasm-opt` 替換至 WSL 的 emsdk 中。

#### 3. 新版 `emcc` 連結選項與特製版 Binaryen 參數不相容（連結失敗）
- **現象**：新版 `emcc` 連結器在調用 `wasm-opt` 和 `wasm-metadce` 時，會自動附加 `--enable-bulk-memory-opt` 與 `--enable-call-indirect-overlong` 參數。但特製版 Binaryen 的版本較舊，不認識這些參數，導致連結中斷。
- **解決方案**：
  - 我們在 WSL 的 `emsdk/upstream/bin/` 目錄中，為 `wasm-opt` 與 `wasm-metadce` 編寫了 **參數過濾 Wrapper 腳本**。
  - Wrapper 會攔截所有命令列引數，過濾掉上述兩個不支持的參數，再將剩下的參數轉交給真正的特製 Binaryen 二進位檔執行。
  - 這完美解決了不相容問題，在 WASMExceptions + Asyncify 的組合下成功完成了極致的二進位優化！

---

### 階段 3：MT-32 音訊品質與效能優化
在首次編譯成功後，發現 MT-32 聲音播放有卡頓、部分遊戲有「粒子感」爆音等問題，我們進行了以下優化：

#### 1. 釋放 CPU 效能（移除 `-Oz`）
- **現象**：在 `Zeliard` 播放音樂到中途會嚴重卡頓數秒，且 `Indianapolis 500` 執行不夠順暢。
- **成因**：Makefile 原先同時設置了 `-O3` 與 `-Oz`。`-Oz` 會為了極限壓縮 binary 大小而停用大量效能優化（如 Inline、Loop Unrolling 等）。然而，Munt 音訊合成是非常消耗 CPU 的密集運算，這導致執行效率低落，運算速度跟不上音訊播放速度。
- **解決方案**：移除 Makefile 中所有的 `-Oz` 旗標，僅保留 `-O3` 進行全速優化編譯，大幅提升運算速度，消除卡頓。

#### 2. 增大前端音訊緩衝區大小（解決粒子感爆音與前台卡頓）
- **現象**：`Budokan` 等遊戲播放時帶有劈啪的「粒子感」爆音，且像 `Indianapolis 500` 等高度消耗 CPU 的遊戲在前台執行時可能偶有卡頓。
- **成因**：前端 `AUDIOBUFFSIZE` 原先設定為 `1024`（約 21ms 的播放延遲時間）。當非同步 WebAssembly 遇到微小執行緒負載波動時，極易發生緩衝區耗盡（underrun），音訊突然中斷歸零，導致連續波形被切斷而產生粒子爆音。
- **解決方案**：將 `AUDIOBUFFSIZE` 增加到 `4096`（約 85ms 的播放延遲時間，人耳幾乎無法察覺），為 Web Audio API 提供四倍的緩衝時間，順利消除了前台卡頓與爆音問題。

#### 3. CPU Timing 平衡調度優化（強制 Yield）
- **現象**：在高 CPU 週期或 `cpu:max` 模式下，模擬器長時間狂奔霸占主執行緒，導致 Web Audio 播送執行緒無暇執行（音訊緩衝區斷炊），進而造成 Adlib 與 MT-32 聲音斷續卡頓。
- **解決方案**：在 `increaseticks()` 中引入時間差判定，若距離上一次 sleep 過去超過 10 毫秒，即強制執行一次 `asyncify_sleep(1, true)` 釋放執行緒，讓出控制權給瀏覽器事件循環，徹底防範音訊斷炊。

---

### 階段 4：解決 CPU Timing 優化引起的 C++ 指令執行 Asyncify 崩潰（Runtime 崩潰與黑屏凍結）
- **現象**：
  1. 當使用者執行 `imgmount` 或 `xcopy` 等 DOS 內建指令時，若在執行中途觸發 10ms 強制 Yield 睡眠，會因為 stack 包含未儀裝的 C++ 函式（如 `MountFat`、`DOS_FindFirst` 等）而導致 `unreachable` 或 `null function` 崩潰。
  2. 若強制將睡眠限制在最外層 `normal_loop_recursion == 1`，則會導致 DOS 殼層在等待鍵盤輸入（此時執行處於 nested `Normal_Loop` 且 `recursion == 2`）時，睡眠被永久繞過，進而使 CPU 陷入死循環、瀏覽器主執行緒卡死，網頁呈現一片漆黑。
- **解決方案**：
  - 放棄依賴容易受 Asyncify stack 影響的 RAII 遞迴深度計數器。
  - 引入全域開關 `asyncify_suspend_disabled`。
  - 在 [programs.cpp](file:///c:/dev/DosWasmX/code/src/misc/programs.cpp#L145-L174) 的 `PROGRAMS_Handler` 中，在程式執行入口將該開關設為 `true`，並於執行結束（含 catch 異常）時還原為原值。
  - 在 `increaseticks()` 與 `Normal_Loop()` 中，只要 `asyncify_suspend_disabled` 為 `true` 就不允許執行 `asyncify_sleep`。
  - 這既能保證在執行內建命令（如 `imgmount`、`xcopy`）等不安全呼叫棧時完全不睡眠，又能保證在 DOS 殼層等待輸入、或普通遊戲執行時允許正常睡眠，完美兼顧了高穩定性、畫面流暢渲染與音訊調度效能！

---

### 階段 5：解決 Firefox 瀏覽器音訊 Autoplay 限制
- **現象**：在 Firefox 或部分瀏覽器中，載入模擬器後完全沒有聲音。
- **成因**：瀏覽器的 Autoplay 安全政策規定，若 `AudioContext` 建立時不處於直接的使用者互動事件 callstack 內，會預設進入 `"suspended"`（暫停）狀態。由於專案在 `FileReader.onload` 非同步回呼後才啟動音訊初始化，因此會被 Firefox 判定為非主動互動而直接禁音。
- **解決方案**：
  - 在 `initAudio()` 中，為 `document` 註冊全域的 `click` 與 `keydown` 事件監聽器。
  - 當使用者點擊網頁或按鍵時，若檢查到 `audioContext.state === 'suspended'`，則主動調用 `audioContext.resume()` 以啟用音訊。
  - 此機制成功繞過瀏覽器限制，使 Firefox 也能流暢播放 MT-32 音效。

---

### 階段 6：配置 GitHub Actions 自動部署流程
- **目標**：為了讓專案能夠快速發佈，配置了 GitHub Actions 自動將 `./dist` 目錄部署至 GitHub Pages 的靜態服務中。
- **解決方案**：
  - 在 `.github/workflows/deploy.yml` 寫入自動部署腳本，並在部署步驟前加入從 archive.org 自動下載 Roland MT-32 ROM 檔案並解壓縮至 `./dist` 的指令（以避免在 Git 倉庫中直接存放有版權爭議的 ROM 檔），每當 `git push` 到 `master` 分支時即會自動觸發部署。
  - 提醒使用者手動於 GitHub 倉庫的 `Settings -> Pages -> Build and deployment -> Source` 改選為 `GitHub Actions` 即可正常運作。
  - 同步更新本地 `run.bat`：若偵測到 `dist/` 目錄下缺少 ROM 檔，會自動從 archive.org 下載並解壓縮，確保本地開發與線上部署均可無縫使用 MT-32。

---

### 階段 7：MT-32 LCD 訊息即時顯示
- **目標**：在網頁標題列右側新增「MT-32 LCD Messages: xxxxxxxxx」即時顯示區塊，讓使用者能看見遊戲透過 MIDI SysEx 傳送給 MT-32 的 LCD 訊息（例如歌曲標題或音色名稱）。
- **C++ 端實作**：
  - 在 [midi_mt32.h](file:///c:/dev/DosWasmX/code/src/gui/midi_mt32.h) 中以 `extern "C"` 區塊定義全域字元陣列 `neil_mt32_lcd_message[128]`，並在 Munt 的 `showLCDMessage()` 回呼中以 `strncpy` 將訊息寫入；於 `Close()` 時清空以避免殘留顯示。
  - 在 [sdlmain.cpp](file:///c:/dev/DosWasmX/code/src/gui/sdlmain.cpp) 中以 `extern char neil_mt32_lcd_message[128]` 宣告並實作 `EMSCRIPTEN_KEEPALIVE const char* neil_get_mt32_lcd_message()`，回傳字串指針供 JavaScript 端輪詢。
  - 在 [Makefile](file:///c:/dev/DosWasmX/code/Makefile) 的 `EXPORTED_FUNCTIONS` 中加入 `'_neil_get_mt32_lcd_message'`。
- **前端實作**：
  - 在 [index.html](file:///c:/dev/DosWasmX/dist/index.html) 的 `<h1>` 標題旁新增 `#mt32-lcd` 容器與 `#mt32-lcd-text` 文字元素，以螢光綠（`#00ffcc`）monospace 字型搭配深色背景展示，預設隱藏。
  - 在 [script.js](file:///c:/dev/DosWasmX/dist/script.js) 中透過 `Module.cwrap` 綁定 `neil_get_mt32_lcd_message`，並在 `configureEmulator()` 後啟動 200ms 輪詢的 `setInterval`，有訊息時顯示 LCD 區塊，空白時隱藏。
- **編譯**：於 WSL 中執行 `make clean && make -j$(nproc)` 完整重新編譯，確保符號表一致，避免增量編譯導致的 function table 偏移問題。

---

### 階段 8：修復 `cwrap` 參數列表錯誤導致的 `function signature mismatch` 崩潰
- **現象**：在加入 MT-32 LCD 功能並重新編譯後，每次啟動模擬器都立即崩潰，出現 `RuntimeError: function signature mismatch at dynCall_viiiii`，定位在 `callMain()` 之後。
- **成因**：`dist/script.js` 中 `neil_send_dos_controls` 的 `Module.cwrap` 宣告：
  ```js
  // 錯誤：7 個參數
  ['string', 'string', 'string', 'array', 'number', 'string', 'string']
  ```
  但 C++ 函式實際只有 **5 個參數**：`controls, axis0, axis1, keyArray, keyLength`。多出的兩個 `'string'` 使 Emscripten 在呼叫時向 stack 推入額外的字串指針，導致 WASM 函式表的簽名比對失敗。  
  此 bug 源於先前某次手動編輯 `script.js` 時不慎多加了兩個型別。
- **解決方案**：將 `cwrap` 的參數列表修正為：
  ```js
  // 正確：5 個參數
  ['string', 'string', 'string', 'array', 'number']
  ```
- **教訓**：`cwrap` 的參數型別列表必須與 C/C++ 函式簽名**完全一致**。任何多餘或缺少的型別都會導致 WASM indirect call table 的簽名校驗失敗，產生難以追蹤的 `function signature mismatch` 崩潰，且錯誤堆疊無法直接指出是哪個函式出錯。在修改 `cwrap` 綁定時，務必對照 C++ 原始碼確認參數數量。

---

## 📈 現有專案架構與維護建議

- **WSL 編譯**：後續若需修改 C++ 代碼，應在 WSL 下載入 `emsdk 3.1.49` 的 `emsdk_env.sh`，並於 `code/` 目錄下執行 `make` 進行編譯。每次修改 C++ 後務必執行 `make clean && make` 以避免增量編譯造成的 function table 偏移。
- **音訊緩衝與效能**：若後續遇到更複雜的 3D 遊戲音訊延遲，可適度調整 `dist/script.js` 中的 `AUDIOBUFFSIZE`，但以 `4096` 為最佳效能平衡點。
- **cwrap 綁定注意事項**：新增或修改 `EMSCRIPTEN_KEEPALIVE` 函式後，必須同步更新 `script.js` 中對應的 `Module.cwrap` 型別列表，並確保參數數量與型別和 C++ 簽名完全一致。
- **MT-32 ROM 管理**：ROM 檔案（`MT32_CONTROL.ROM`、`MT32_PCM.ROM`）不應提交至 Git 倉庫。本地使用 `run.bat` 自動下載，線上部署透過 GitHub Actions 自動下載。
