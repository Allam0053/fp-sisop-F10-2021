# fp-sisop-F10-2021
How application works:
1. server berjalan di background (daemon)
2. akses console database -> jalananin program client (mirip buka mysql/postgres di terminal).
3. program client utama berinteraksi lewat socket
4. program client bisa mengakses server dari mana saja (apakah maksudnya dari dir mana saja ?)

Yang masih bingung:
1. struktur folder server ? -> kata zaki ada di dalam database/



// TODO: 
1. Saat program dirun, buat agar bisa menerima argumen -u <user> -p <password>
2. Buat folder db_user untuk menyimpan user & password & hak akses db nya
    Note:
    - db_user hanya bisa diakses user 'root' (sudo ./program)
    - user 'root' sudah ada dari awal
  

Setelah program dijalankan:
3. Buat program agar bisa membuat user baru 
    => CREATE USER <nama user>;
    Note:
    - CREATE USER hanya bisa dilakukan user 'root'

4. Buat agar si user bisa connect ke db 
    => USE <nama_db>;
    Note:
    - koneksi db hanya diizinkan jika user memang memiliki akses ke db nya
    - DML suatu db, db hanya harus ada terlebih dahulu

5. Buat agar bisa grant permission suatu db ke suatu user 
    => GRANT PERMISSION database1 INTO user1;
    Note:
    - GRANT PERMISSION hanya bisa dilakukan user 'root'

6. Buat agar bisa membuat database baru 
    => CREATE DATABASE <nama database>;
    Note:
    - user yang membuat database akan default punya hak akses ke database tersebut

7. Buat agar bisa create tabel + kolom2 nya 
    => CREATE TABLE [nama_tabel] ([nama_kolom] [tipe_data], ...);
    Note:
    - Tipe data kolom hanya int atau string saja

8. Buat agar bisa drop database, tabel, dan kolom
    => DROP DATABASE <nama db>;
    => DROP TABLE <nama tabel>;
    => DROP COLUMN <nama column> FROM <nama tabel>;
    Note:
    - untuk drop tabel, harus sudah terkoneksi ke suatu db
    - Kalo db / tabel / kolom gak ada, yaudah biarin aja



9. Buat agar bisa insert data ke suatu tabel
    => INSERT INTO table1 ('string1’, 2, 'string2’, 4);
    Note:
    - Urutan kolom & tipe data harus sesuai
    - Hanya bisa 1x per 1 command

10. Buat agar bisa update suatu kolom 
    => UPDATE table1 SET kolom1=’new_value1’;
    Note:
    - Hanya bisa update satu kolom per satu command.

11. Buat agar bisa delte data
    => DELETE FROM table1;

12. Buat agar bisa menampilkan data-data
    => SELECT * FROM table1;
    => SELECT kolom1, kolom2 FROM table1;

13. Buat agar bisa filtering pake WHERE
    => format: [Command UPDATE, SELECT, DELETE] WHERE [nama_kolom]=[value];
    Note:
    - Hanya untuk 1 kondisi dengan operator '=' saja




14. Buat agar ada pencatatan log ke suatu file untuk setiap command yang dipakai
    => 2021-05-19 02:05:15:jack:SELECT FROM table1

15. dump database ???

16. Buat program bisa menerima input COMMAND SQL dari file
    => ./[program_client_database] -u [username] -p [password] -d [database] < [file_command]