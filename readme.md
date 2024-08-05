## 実装した加点要素
- 球の描画    
- Lambert反射    
- half Lambert
- UVtransform
- 複数モデルの描画
- teapotの描画
- bunnyの描画
- multiMeshの対応
- multiMaterialの対応
- suzanneの描画
- Lightingの変更

## 操作，要素の確認
ImGUI最上段の"scene"から要素の変更を行う<br>
"plane","sprote"等のTagから各オブジェクトの状態を変更できる

### 各シーンで確認できること
- obj & sprite
    - spriteのUVtransform
- sphere
    - 球の描画
    - Lambert反射    
    - half Lambert
    - Lightingの変更
        - "sphere"タグの"Lightting"チェックボックス->lighttingの有無
        - "directionalLight"タグの"enable half"チェックボックス->hlafLambertの有無
- obj & obj
    - 複数モデルの描画
    - teapotの描画
    - bunnyの描画
- suzanne
    - suzanneの描画
- multiMesh
    - multiMeshの対応
- multiMaterial
    - multiMaterialの対応

### その他追加要素
 - アルファ(透明度)の適応<br>->常時適応
 - phongの実装<br>->常時適応<br>
 <br>
 以上