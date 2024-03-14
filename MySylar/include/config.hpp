/*
 * @Description: 
 * @Version: 1.0
 * @Author: LOSER
 * @Date: 2024-01-29 15:04:33
 * @LastEditors: LOSER
 * @LastEditTime: 2024-03-01 17:29:25
 */

#ifndef __CONFIG__HPP__
#define __CONFIG__HPP__


#include<boost/lexical_cast.hpp>

#include <memory>
#include <unordered_set>
#include <unordered_map>

#include <log.hpp>
#include <env.hpp>
#include <mutex.hpp>

namespace MySylar {

static Logger::loggerPtr g_logger = LOG_NAME("SYSTEM");

/// @brief 配置变量基类
class ConfigVarBase {
public:
    using ConfigVarBasePtr = std::shared_ptr<ConfigVarBase>;

public:
    ConfigVarBase(const std::string &name, const std::string & descripation = "unknow") 
    :_name(name)
    ,_descripation(descripation){
        //转成小写
        std::transform(_name.begin(), _name.end(), _name.begin(), ::tolower);
    }

    virtual ~ConfigVarBase() {}

    const std::string &GetName() const {return _name;}

    const std::string &GetDescription() const {return _descripation;}

    virtual std::string ToString() = 0;

    virtual bool FromString(const std::string &val) = 0;

    virtual std::string GetTypeName() const = 0;


protected:
    std::string _name;

    std::string _descripation;
};

/// @brief 类型转换模板
/// @tparam F 原类型
/// @tparam T 目标类型
template <class F, class T>
class TypeLexicakCast {

public:
    T operator()(const F & v) {
        return boost::lexical_cast<T>(v);
    }
};

/**
 * @brief 类型转换模板类片特化(YAML String 转换成 std::vector<T>)
 */
template <class T>
class TypeLexicakCast<std::string, std::vector<T>>{

public:
    std::vector<T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        typename std::vector<T> vec;
        std::stringstream ss;
        for(size_t i = 0; i < node.size(); ++i) {
            ss.str("");//清空
            ss << node[i];
            vec.emplace_back(TypeLexicakCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};


/// @brief 类型转换模板类片特化(std::vector<T> 转换成 YAML String)
/// @tparam T 
template<class T>
class TypeLexicakCast<std::vector<T>, std::string> {

public:
    std::string operator()(const std::vector<T> & v) {
        YAML::Node node(YAML::NodeType::Sequence);
        for(auto &i : v) {
            node.push_back(YAML::Load(TypeLexicakCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/**
 * @brief 类型转换模板类片特化(YAML String 转换成 std::list<T>)
 */
template <class T>
class TypeLexicakCast<std::string, std::list<T>>{

public:
    std::list<T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        typename std::list<T> vec;
        std::stringstream ss;
        for(size_t i = 0; i < node.size(); ++i) {
            ss.str("");//清空
            ss << node[i];
            vec.emplace_back(TypeLexicakCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

/// @brief 类型转换模板类片特化(std::List<T> 转换成 YAML String)

template<class T>
class TypeLexicakCast<std::list<T>, std::string> {

public:
    std::string operator()(const std::list<T> & v) {
        YAML::Node node(YAML::NodeType::Sequence);
        for(auto &i : v) {
            node.push_back(YAML::Load(TypeLexicakCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/**
 * @brief 类型转换模板类片特化(YAML String 转换成 std::set<T>)
 */
template <class T>
class TypeLexicakCast<std::string, std::set<T>>{

public:
    std::vector<T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        typename std::set<T> vec;
        std::stringstream ss;
        for(size_t i = 0; i < node.size(); ++i) {
            ss.str("");//清空
            ss << node[i];
            vec.emplace_back(TypeLexicakCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

/**
 * @brief 类型转换模板类片特化(std::set<T> 转换成 YAML String)
 **/
template <class T>
class TypeLexicakCast<std::set<T>, std::string> {
public:
    std::string operator()(const std::set<T> &v) {
        YAML::Node node(YAML::NodeType::Sequence);
        for (auto &i : v) {
            node.push_back(YAML::Load(TypeLexicakCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};/**
 * @brief 类型转换模板类片特化(YAML String 转换成 std::unordered_set<T>)
 */
template <class T>
class TypeLexicakCast<std::string, std::unordered_set<T>> {
public:
    std::unordered_set<T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        typename std::unordered_set<T> vec;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.insert(TypeLexicakCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

/**
 * @brief 类型转换模板类片特化(std::unordered_set<T> 转换成 YAML String)
 */
template <class T>
class TypeLexicakCast<std::unordered_set<T>, std::string> {
public:
    std::string operator()(const std::unordered_set<T> &v) {
        YAML::Node node(YAML::NodeType::Sequence);
        for (auto &i : v) {
            node.push_back(YAML::Load(TypeLexicakCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/**
 * @brief 类型转换模板类片特化(YAML String 转换成 std::map<std::string, T>)
 */
template <class T>
class TypeLexicakCast<std::string, std::map<std::string, T>> {
public:
    std::map<std::string, T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        typename std::map<std::string, T> vec;
        std::stringstream ss;
        for (auto it = node.begin();
             it != node.end(); ++it) {
            ss.str("");
            ss << it->second;
            vec.insert(std::make_pair(it->first.Scalar(),
                                      TypeLexicakCast<std::string, T>()(ss.str())));
        }
        return vec;
    }
};

/**
 * @brief 类型转换模板类片特化(std::map<std::string, T> 转换成 YAML String)
 */
template <class T>
class TypeLexicakCast<std::map<std::string, T>, std::string> {
public:
    std::string operator()(const std::map<std::string, T> &v) {
        YAML::Node node(YAML::NodeType::Map);
        for (auto &i : v) {
            node[i.first] = YAML::Load(TypeLexicakCast<T, std::string>()(i.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/**
 * @brief 类型转换模板类片特化(YAML String 转换成 std::unordered_map<std::string, T>)
 */
template <class T>
class TypeLexicakCast<std::string, std::unordered_map<std::string, T>> {
public:
    std::unordered_map<std::string, T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        typename std::unordered_map<std::string, T> vec;
        std::stringstream ss;
        for (auto it = node.begin();
             it != node.end(); ++it) {
            ss.str("");
            ss << it->second;
            vec.insert(std::make_pair(it->first.Scalar(),
                                      TypeLexicakCast<std::string, T>()(ss.str())));
        }
        return vec;
    }
};

/**
 * @brief 类型转换模板类片特化(std::unordered_map<std::string, T> 转换成 YAML String)
 */
template <class T>
class TypeLexicakCast<std::unordered_map<std::string, T>, std::string> {
public:
    std::string operator()(const std::unordered_map<std::string, T> &v) {
        YAML::Node node(YAML::NodeType::Map);
        for (auto &i : v) {
            node[i.first] = YAML::Load(TypeLexicakCast<T, std::string>()(i.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};


template <class T, class FromStr = TypeLexicakCast<std::string, T>, 
            class ToStr = TypeLexicakCast<T, std::string>>
class ConfigVar : public ConfigVarBase {
public:
    using RWMutexType = RWMutex;

    using onChangeCb = std::function<void(const T, const T)>;

    using configVarPtr = std::shared_ptr<ConfigVar>;
public:
    
    ConfigVar(const std::string &name, const T & defaultValue, const std::string &description = "unkonw")
    : ConfigVarBase(name, description)
    , _val(defaultValue) {

    }

    std::string ToString() override {
        try{
            RWMutexType::ReadLock lock(_rwMutex);
            return ToStr()(_val);
        }catch(std::exception & e) {
            ERROR_LOG(LOG_ROOT()) << "ConfigVar::ToString exception"
                                << e.what() << "convert: " << TypeToName<T>() << " to string"
                                << " name=" << _name;
        }
        return "";
    }

        /**
     * @brief 从YAML String 转成参数的值
     * @exception 当转换失败抛出异常
     */
    bool FromString(const std::string &val) override {
        try {
            SetValue(FromStr()(val));
        } catch (std::exception &e) {
            ERROR_LOG(LOG_ROOT()) << "ConfigVar::fromString exception "
                                              << e.what() << " convert: string to " << TypeToName<T>()
                                              << " name=" << _name
                                              << " - " << val;
        }
        return false;
    }

    const T GetValue() {
        RWMutexType::ReadLock lock(_rwMutex);
        return _val;
    }

        /**
     * @brief 设置当前参数的值
     * @details 如果参数的值有发生变化,则通知对应的注册回调函数
     */
    void SetValue(const T &v) {
        {
            RWMutexType::ReadLock lock(_rwMutex);
            if (v == _val) {
                return;
            }
            for (auto &i : _cbs) {
                i.second(_val, v);
            }
        }
        RWMutexType::WriteLock lock(_rwMutex);
        _val = v;
    }

        /**
     * @brief 返回参数值的类型名称(typeinfo)
     */
    std::string GetTypeName() const override { return TypeToName<T>(); }

        uint64_t addListener(onChangeCb cb) {
        static uint64_t s_fun_id = 0;
        RWMutexType::WriteLock lock(_rwMutex);
        ++s_fun_id;
        _cbs[s_fun_id] = cb;
        return s_fun_id;
    }

    /**
     * @brief 删除回调函数
     * @param[in] key 回调函数的唯一id
     */
    void delListener(uint64_t key) {
        RWMutexType::WriteLock lock(_rwMutex);
        _cbs.erase(key);
    }

    /**
     * @brief 获取回调函数
     * @param[in] key 回调函数的唯一id
     * @return 如果存在返回对应的回调函数,否则返回nullptr
     */
    onChangeCb getListener(uint64_t key) {
        RWMutexType::ReadLock lock(_rwMutex);
        auto it = _cbs.find(key);
        return it == _cbs.end() ? nullptr : it->second;
    }

    /**
     * @brief 清理所有的回调函数
     */
    void clearListener() {
        RWMutexType::WriteLock lock(_rwMutex);
        _cbs.clear();
    }


private:
    RWMutexType _rwMutex;

    T _val;
    std::map<uint64_t, onChangeCb> _cbs;
};

static void ListAllMember(const std::string &prefix,
                          const YAML::Node &node,
                          std::list<std::pair<std::string, const YAML::Node>> &output) {
    if (prefix.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._012345678") != std::string::npos) {
        ERROR_LOG(LOG_ROOT()) << "Config invalid name: " << prefix << " : " << node;
        return;
    }
    output.push_back(std::make_pair(prefix, node));
    if (node.IsMap()) {
        for (auto it = node.begin();
             it != node.end(); ++it) {
            ListAllMember(prefix.empty() ? it->first.Scalar()
                                         : prefix + "." + it->first.Scalar(),
                          it->second, output);
        }
    }
}


/// 记录每个文件的修改时间
static std::map<std::string, uint64_t> s_file2modifytime;
/// 是否强制加载配置文件，非强制加载的情况下，如果记录的文件修改时间未变化，则跳过该文件的加载
static MySylar::Mutex s_mutex;

/**
 * @brief ConfigVar的管理类
 * @details 提供便捷的方法创建/访问ConfigVar
 */
class Config {
public:
    using ConfigVarMap = std::unordered_map<std::string, ConfigVarBase::ConfigVarBasePtr>;
    typedef RWMutex RWMutexType;

    /**
     * @brief 获取/创建对应参数名的配置参数
     * @param[in] name 配置参数名称
     * @param[in] default_value 参数默认值
     * @param[in] description 参数描述
     * @details 获取参数名为name的配置参数,如果存在直接返回
     *          如果不存在,创建参数配置并用default_value赋值
     * @return 返回对应的配置参数,如果参数名存在但是类型不匹配则返回nullptr
     * @exception 如果参数名包含非法字符[^0-9a-z_.] 抛出异常 std::invalid_argument
     */
    template <class T>
    static typename ConfigVar<T>::configVarPtr Lookup(const std::string &name,
                                             const T &default_value, const std::string &description = "unknow") {
        RWMutexType::WriteLock lock(GetMutex());
        auto it = GetDatas().find(name);
        if (it != GetDatas().end()) {
            auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
            if (tmp) {
                INFO_LOG(LOG_ROOT()) << "Lookup name=" << name << " exists";
                return tmp;
            } else {
                ERROR_LOG(LOG_ROOT()) << "Lookup name=" << name << " exists but type not "
                                                  << TypeToName<T>() << " real_type=" << it->second->GetTypeName()
                                                  << " " << it->second->ToString();
                return nullptr;
            }
        }

        if (name.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._012345678") != std::string::npos) {
            ERROR_LOG(LOG_ROOT()) << "Lookup name invalid " << name;
            throw std::invalid_argument(name);
        }

        typename ConfigVar<T>::configVarPtr v(new ConfigVar<T>(name, default_value, description));
        GetDatas()[name] = v;
        return v;
    }

    /**
     * @brief 查找配置参数
     * @param[in] name 配置参数名称
     * @return 返回配置参数名为name的配置参数
     */
    template <class T>
    static typename ConfigVar<T>::configVarPtr Lookup(const std::string &name) {
        RWMutexType::ReadLock lock(GetMutex());
        auto it = GetDatas().find(name);
        if (it == GetDatas().end()) {
            return nullptr;
        }
        return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
    }

    /**
     * @brief 使用YAML::Node初始化配置模块
     */
    static void LoadFromYaml(const YAML::Node &root) {
        std::list<std::pair<std::string, const YAML::Node>> all_nodes;
        ListAllMember("", root, all_nodes);

        for (auto &i : all_nodes) {
            std::string key = i.first;
            if (key.empty()) {
                continue;
            }

            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            ConfigVarBase::ConfigVarBasePtr var = LookupBase(key);

            if (var) {
                if (i.second.IsScalar()) {
                    var->FromString(i.second.Scalar());
                } else {
                    std::stringstream ss;
                    ss << i.second;
                    var->FromString(ss.str());

                }
            }
        }
    }

    /**
     * @brief 加载path文件夹里面的配置文件
     */
    static void LoadFromConfDir(const std::string &path, bool force = false) {
        std::string absoulte_path = singelEnv::GetInstance()->GetAbsolutePath(path);
        std::vector<std::string> files;
        FSUtil::ListAllFile(files, absoulte_path, ".yml");



        for (auto &i : files) {
            {
                struct stat st;
                lstat(i.c_str(), &st);
                MySylar::Mutex::lock tempLock(s_mutex);
                if (!force && s_file2modifytime[i] == (uint64_t)st.st_mtime) {
                    continue;
                }
                s_file2modifytime[i] = st.st_mtime;
            }
            try {
                YAML::Node root = YAML::LoadFile(i);
                LoadFromYaml(root);
                INFO_LOG(g_logger) << "LoadConfFile file="
                                        << i << " ok";
            } catch (...) {
                ERROR_LOG(g_logger) << "LoadConfFile file="
                                        << i << " failed";
            }
        }
    }

    /**
     * @brief 查找配置参数,返回配置参数的基类
     * @param[in] name 配置参数名称
     */
    static ConfigVarBase::ConfigVarBasePtr LookupBase(const std::string &name) {
        RWMutexType::ReadLock lock(GetMutex());
        auto it = GetDatas().find(name);
        return it == GetDatas().end() ? nullptr : it->second;
    }

    /**
     * @brief 遍历配置模块里面所有配置项
     * @param[in] cb 配置项回调函数
     */
    static void Visit(std::function<void(ConfigVarBase::ConfigVarBasePtr)> cb) {
        RWMutexType::ReadLock lock(GetMutex());
        ConfigVarMap &m = GetDatas();
        for (auto it = m.begin();
            it != m.end(); ++it) {
            cb(it->second);
        }
    }

private:
    /**
     * @brief 返回所有的配置项
     */
    static ConfigVarMap &GetDatas() {
        static ConfigVarMap s_datas;
        return s_datas;
    }

    /**
     * @brief 配置项的RWMutex
     */
    static RWMutexType &GetMutex() {
        static RWMutexType s_mutex;
        return s_mutex;
    }
};

} // namespace MySylar

#endif