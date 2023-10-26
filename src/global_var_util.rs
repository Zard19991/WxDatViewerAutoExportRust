// use lazy_static::lazy_static;
use rusqlite::Connection;
// use once_cell::sync::OnceCell;
// use std::cell::LazyCell;
use std::collections::HashMap;
use std::sync::atomic::AtomicUsize;
use std::sync::atomic::Ordering;
use std::sync::Arc;
use std::sync::Mutex;
use std::sync::MutexGuard;
use std::sync::OnceLock;

use crate::global_var::insert_vec_string;
use crate::global_var::push_vec_string;

// lazy_static! {
//     static ref VARIABLE_VEC_EXPORT_DIR_ITME: Mutex<Vec<ExportDirItme>> = Mutex::new(Vec::new());
// }

static mut VARIABLE_VEC_EXPORT_DIR_ITME: Vec<ExportDirItme> = Vec::new();
static VARIABLE_VEC_EXPORT_DIR_ITME_BIND: AtomicUsize = AtomicUsize::new(0);

static VARIABLE_INITIALIZE: OnceLock<bool> = OnceLock::new();
static INITIALIZE_GET_EXPORT_DIR_TIME_LIST: OnceLock<bool> = OnceLock::new();
#[derive(Debug)]
// 用户任务
pub struct ExportDirItme {
    pub id: i32,
    pub time: String,
    pub name: String,
    pub path: String,
    pub ouput: String,
}

impl Clone for ExportDirItme {
    fn clone(&self) -> Self {
        ExportDirItme {
            id: self.id.clone(),
            time: self.time.clone(),
            name: self.name.clone(),
            path: self.path.clone(),
            ouput: self.ouput.clone(),
        }
    }
}

// 从数据库中获取用户任务
fn get_export_dir_path_itme_sql_lib(
    conn: &Connection,
) -> Result<Vec<ExportDirItme>, rusqlite::Error> {
    let mut result: Vec<ExportDirItme> = Vec::new();
    let mut stmt = conn.prepare("SELECT id, time, name, path, ouput  FROM export_dir_path")?;

    let cats = stmt.query_map([], |row| {
        Ok(ExportDirItme {
            id: row.get(0)?,
            time: row.get(1)?,
            name: row.get(2)?,
            path: row.get(3)?,
            ouput: row.get(4)?,
        })
    })?;

    for cat in cats {
        let paths = cat?;
        result.push(paths);
    }

    Ok(result)
}

// 获取用户任务 （刷新）
pub fn update_export_dir_itme_list() -> Vec<ExportDirItme> {
    let mut itme_list: Vec<ExportDirItme> = Vec::new();

    let conn: Connection = match Connection::open("ikun_user_data.db") {
        Ok(conn) => conn,
        Err(e) => {
            push_vec_string(
                "console_log",
                format!("[用户任务] 数据库内部错误因为 ->  {:?}", e),
            );
            return Vec::new();
        }
    };

    match get_export_dir_path_itme_sql_lib(&conn) {
        Ok(data) => {
            for value in data {
                itme_list.push(value);
            }
        }
        Err(e) => {}
    }

    match conn.close() {
        Ok(_) => {}
        Err(e) => {
            push_vec_string(
                "console_log",
                format!("[用户任务] 数据库断开错误因为 ->  {:?}", e),
            );
        }
    };

    // let mut lazy_value = VARIABLE_VEC_EXPORT_DIR_ITME.lock().unwrap();
    // let mut oid_len: i32 = lazy_value.len().try_into().unwrap();

    // lazy_value.clear();

    let mutex = Arc::new(Mutex::new(&VARIABLE_VEC_EXPORT_DIR_ITME_BIND));
    let _ = mutex.lock();
    let the_value: usize = VARIABLE_VEC_EXPORT_DIR_ITME_BIND.load(Ordering::SeqCst);
    let mut oid_len: i32 = 0;

    unsafe {
        oid_len = VARIABLE_VEC_EXPORT_DIR_ITME.len() as i32;
        VARIABLE_VEC_EXPORT_DIR_ITME.clear();
        for value in &itme_list {
            VARIABLE_VEC_EXPORT_DIR_ITME.push(value.clone());
        }
    };

    VARIABLE_VEC_EXPORT_DIR_ITME_BIND.store(the_value + 1, Ordering::SeqCst);
    drop(mutex);

    let itme_list_len: i32 = itme_list.len().try_into().unwrap();

    if (oid_len != itme_list_len) {
        push_vec_string(
            "console_log",
            format!(
                "[数据更新] 用户任务列表数量 -> {} 数量{}",
                itme_list_len,
                itme_list_len - oid_len
            ),
        );
    }

    return itme_list;
}

// 获取用户任务 （不刷新 除非为0）
pub fn get_export_dir_itme_list() -> Vec<ExportDirItme> {
    if !*(INITIALIZE_GET_EXPORT_DIR_TIME_LIST
        .get()
        .unwrap_or_else(|| &false))
    {
        INITIALIZE_GET_EXPORT_DIR_TIME_LIST.set(true);
        return update_export_dir_itme_list();
    }

    let mut itme_list: Vec<ExportDirItme> = Vec::new();

    let mutex = Arc::new(Mutex::new(&VARIABLE_VEC_EXPORT_DIR_ITME_BIND));
    let _ = mutex.lock();
    let the_value: usize = VARIABLE_VEC_EXPORT_DIR_ITME_BIND.load(Ordering::SeqCst);

    let data = unsafe {
        for value in &VARIABLE_VEC_EXPORT_DIR_ITME {
            itme_list.push(value.clone());
        }
    };
    VARIABLE_VEC_EXPORT_DIR_ITME_BIND.store(the_value + 1, Ordering::SeqCst);
    drop(mutex);

    return itme_list;
}

// 获取用户任务 （不刷新 除非为0）
pub fn get_export_dir_itme_len() -> usize {
    if !*(INITIALIZE_GET_EXPORT_DIR_TIME_LIST
        .get()
        .unwrap_or_else(|| &false))
    {
        INITIALIZE_GET_EXPORT_DIR_TIME_LIST.set(true);
        return update_export_dir_itme_list().len();
    }
    let mut itme_list_len = 0;

    let mutex = Arc::new(Mutex::new(&VARIABLE_VEC_EXPORT_DIR_ITME_BIND));
    let _ = mutex.lock();
    let the_value: usize = VARIABLE_VEC_EXPORT_DIR_ITME_BIND.load(Ordering::SeqCst);

    let data = unsafe { itme_list_len = VARIABLE_VEC_EXPORT_DIR_ITME.len() };

    VARIABLE_VEC_EXPORT_DIR_ITME_BIND.store(the_value + 1, Ordering::SeqCst);
    drop(mutex);

    return itme_list_len;
}

// 获取指定数量
pub fn get_group_export_task_value(index: i32, len: usize) -> Vec<ExportDirItme> {
    if let Some(item) = get_group_export_task_value_list(len).get(index as usize) {
        return item.clone();
    }

    Vec::new()
}

// 拆分导出类别为xx一组方便计算
pub fn get_group_export_task_value_list(len: usize) -> Vec<Vec<ExportDirItme>> {
    let mut list = Vec::new();
    let mut export_dir_path_list = get_export_dir_itme_list();

    let mut temp = Vec::new();

    for value in export_dir_path_list {
        temp.push(value);
        if temp.len() >= len {
            list.push(temp.clone());
            temp.clear();
        }
    }

    if !temp.is_empty() {
        list.push(temp);
    }

    list
}

pub fn get_export_dir_itme_from_id(id: i32) -> Option<ExportDirItme> {
    for value in get_export_dir_itme_list() {
        if value.id.eq(&id.to_owned()) {
            return Some(value);
        }
    }

    None
}

#[derive(Debug)]
// 用户任务
pub struct ExportDirItemThumbnail {
    pub id: i32,
    pub time: String,
    pub name: String,
    pub path: String,
    pub ouput: String,
    pub thumbnail:Vec<u8>
}

impl Clone for ExportDirItemThumbnail {
    fn clone(&self) -> Self {
        ExportDirItemThumbnail {
            id: self.id.clone(),
            time: self.time.clone(),
            name: self.name.clone(),
            path: self.path.clone(),
            ouput: self.ouput.clone(),
            thumbnail: self.thumbnail.clone(),
        }
    }
}

pub fn get_thumbnail_from_id (id:i32) -> Result<Vec<ExportDirItemThumbnail>, rusqlite::Error> {
    
    let conn: Connection = match Connection::open("ikun_user_data.db") {
        Ok(conn) => conn,
        Err(e) => {
            return Ok(Vec::new());
        }
    };

    let mut stmt = conn.prepare("SELECT id, time, name, path, ouput, thumbnail  FROM export_dir_path WHERE id = ?1")?;
    let mut res = Vec::new();

    let cats = stmt.query_map([id], |row| {
        Ok(ExportDirItemThumbnail {
            id: row.get(0)?,
            time: row.get(1)?,
            name: row.get(2)?,
            path: row.get(3)?,
            ouput: row.get(4)?,
            thumbnail: row.get_ref(5)?.as_bytes()?.to_vec()
        })
    })?;

    for cat in cats {
        let value = cat?;
        res.push(value);
    }

    Ok(res)
}

pub fn get_export_from_id_thumbnail(id: i32) -> Vec<u8> {
    
    if let Some(export_dir_itme) = get_export_dir_itme_from_id(id) {        
        if let Ok(item) = get_thumbnail_from_id(export_dir_itme.id) {
            if let Some(item) = item.get(0) {
             
                return item.thumbnail.to_vec();
            }
        }
    }

    include_bytes!("./GUI/gui_task_manage/src/task_icon.png").to_vec()
}


pub fn set_export_from_id_thumbnail(id: i32,thumbnail:Option<Vec<u8>>)-> Result<(), rusqlite::Error> {
    let conn: Connection = Connection::open("ikun_user_data.db")?;
    let mut stmt = conn.execute(
        "UPDATE export_dir_path SET thumbnail = ? WHERE id = ?",
        rusqlite::params![thumbnail, id],
    )?;
    update_export_dir_itme_list();
    conn.close();
    Ok(())
}

pub fn insert_export_from_id_thumbnail1(input:ExportDirItemThumbnail)-> Result<(), rusqlite::Error> {
   
    let conn: Connection = Connection::open("ikun_user_data.db")?;
    let mut stmt = conn.execute(
        "INSERT INTO export_dir_path (id,time, name, path, ouput, thumbnail)
        VALUES (?, ?, ?, ?, ?, ?)",
        rusqlite::params![input.id,input.time, input.name, input.path, input.ouput, input.thumbnail],
    )?;
    update_export_dir_itme_list();
    conn.close();
    Ok(())
}

pub fn insert_export_from_id_thumbnail2(input:ExportDirItme)-> Result<(), rusqlite::Error> {
   
    let conn: Connection = Connection::open("ikun_user_data.db")?;
    let mut stmt = conn.execute(
        "INSERT INTO export_dir_path (id,time, name, path, ouput)
        VALUES (?, ?, ?, ?, ?, ?)",
        rusqlite::params![input.id,input.time, input.name, input.path, input.ouput],
    )?;
    update_export_dir_itme_list();
    conn.close();
    Ok(())
}

pub fn update_export_from_id_thumbnail(id: i32,name:&str,path:&str,ouput:&str,thumbnail:Option<Vec<u8>>)-> Result<(), rusqlite::Error> {
    let conn: Connection = Connection::open("ikun_user_data.db")?;
   if thumbnail.is_some() {
    let mut stmt = conn.execute(
        "UPDATE export_dir_path SET name = ? , path = ? , ouput = ? ,thumbnail = ? WHERE id = ?",
        rusqlite::params![name,path,ouput,thumbnail, id],
    )?;

   }else{
   
    let mut stmt = conn.execute(
        "UPDATE export_dir_path SET name = ? , path = ? , ouput = ?  WHERE id = ?",
        rusqlite::params![name,path,ouput, id],
    )?;

   }
   
    update_export_dir_itme_list();
    conn.close();
    Ok(())
}