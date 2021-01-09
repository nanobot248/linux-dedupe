{
    'targets': [
        {
            'target_name': 'linux-dedupe',
            'sources': [ 'src/cxx/linux-dedupe.cpp' ],
            'include_dirs': [
                '<!(node -e "require(\'nan\')")'
            ]
        }
    ]
}